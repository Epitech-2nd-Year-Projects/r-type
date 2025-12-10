#include "server_runtime.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/reliability_policy.h"
#include "protocol/world_snapshot.h"

namespace server {
constexpr std::uint32_t kPeerTimeoutMs = 15'000;
constexpr std::uint32_t kReliableResendTimeoutMs = 250;
constexpr std::size_t kReliableQueueMaxPending = 64;

namespace {

using protocol::message_type::MessageType;

std::atomic_bool g_shutdown_requested{false};

void SignalHandler(int) { g_shutdown_requested.store(true); }

std::string EndpointKey(const engine::net::Endpoint& endpoint) {
  std::string key = endpoint.address();
  key.append(":");
  key.append(std::to_string(endpoint.port()));
  return key;
}

std::uint32_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(now).count());
}

void InstallSignalHandlers() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
}

}  // namespace

ServerRuntime::ServerRuntime(ServerConfig config)
    : config_(std::move(config)),
      logger_(engine::util::Logger::Default()),
      frame_timer_(static_cast<float>(config_.tick_rate)),
      rng_(config_.seed),
      game_instance_(config_.seed),
      fixed_delta_(engine::time::TimeDelta::from_seconds(
          1.0f /
          static_cast<float>(config_.tick_rate > 0 ? config_.tick_rate : 60))),
      accumulator_(engine::time::TimeDelta::zero()) {}

std::error_code ServerRuntime::Start() {
  ConfigureLogging();
  InstallSignalHandlers();

  if (const auto start_error = transport_.Start(config_.port); start_error) {
    return start_error;
  }

  logger_.get().Info("Server listening on ",
                     transport_.local_endpoint().address());
  const std::string room_label =
      config_.room_code.empty() ? std::string{"<any>"} : config_.room_code;
  logger_.get().Info("Room ", room_label, " max players ", config_.max_players,
                     " tickrate ", config_.tick_rate, " seed ", config_.seed);
  return {};
}

void ServerRuntime::Run() { RunMainLoop(); }

void ServerRuntime::RunMainLoop() {
  running_ = true;
  accumulator_ = engine::time::TimeDelta::zero();

  while (running_ && !g_shutdown_requested.load()) {
    const auto delta = frame_timer_.tick();
    accumulator_ += delta;
    PollNetwork();
    ProcessReliableResends();

    while (accumulator_ >= fixed_delta_) {
      PollNetwork();
      ProcessReliableResends();
      game_instance_.Update(fixed_delta_);
      ++server_tick_;
      BroadcastWorldSnapshot();
      accumulator_ -= fixed_delta_;
    }
    CheckPeerTimeouts();
    TickRateSleep(delta);
  }
  running_ = false;
}

void ServerRuntime::ConfigureLogging() {
  logger_.get().SetName("server");
  logger_.get().SetLevel(config_.log_level);
}

void ServerRuntime::PollNetwork() {
  const auto poll = transport_.PollNetwork();
  for (auto& datagram : poll.datagrams) {
    HandlePacket(std::move(datagram.payload), datagram.from);
  }
  if (poll.error) {
    logger_.get().Error("Receive error: ", poll.error.message());
    running_ = false;
  }
}

void ServerRuntime::TickRateSleep(const engine::time::TimeDelta& delta_time) {
  const float target_ms =
      1000.0f /
      static_cast<float>(config_.tick_rate > 0 ? config_.tick_rate : 1);
  const float delta_ms = delta_time.as_milliseconds();
  if (delta_ms >= target_ms) {
    return;
  }
  const auto sleep_ms =
      std::chrono::milliseconds(static_cast<int>(target_ms - delta_ms));
  if (sleep_ms.count() > 0) {
    std::this_thread::sleep_for(sleep_ms);
  }
}

void ServerRuntime::HandlePacket(engine::net::PacketBuffer packet,
                                 const engine::net::Endpoint& from) {
  protocol::Packet decoded{};
  protocol::DecodeError error{protocol::DecodeError::kOk};

  if (!protocol::DecodePacket(packet, decoded, error)) {
    protocol::UpdateDecodeMetrics(decode_metrics_, error);
    logger_.get().Warn("Dropped packet from ", EndpointKey(from), " (",
                       protocol::DecodeErrorToString(error),
                       ") total=", decode_metrics_.total_packets,
                       " rejected=", decode_metrics_.rejected_packets);
    return;
  }
  protocol::UpdateDecodeMetrics(decode_metrics_, protocol::DecodeError::kOk);
  PeerConnection& peer = GetOrCreatePeer(from);
  peer.last_activity_ms = NowMilliseconds();
  peer.sequence_tracker.OnRemoteSequenceReceived(decoded.header.sequence);
  ProcessPeerAcks(peer, decoded.header);

  const auto type = static_cast<MessageType>(decoded.header.message_type);

  switch (type) {
    case MessageType::kJoinRequest: {
      if (!std::holds_alternative<protocol::JoinRequestPayload>(
              decoded.payload)) {
        logger_.get().Warn("Malformed join request from ", peer.endpoint_key);
        return;
      }
      ProcessJoin(peer,
                  std::get<protocol::JoinRequestPayload>(decoded.payload));
      break;
    }
    case MessageType::kPing: {
      if (!std::holds_alternative<protocol::PingPayload>(decoded.payload)) {
        logger_.get().Warn("Malformed ping from ", peer.endpoint_key);
        return;
      }
      HandlePing(peer, std::get<protocol::PingPayload>(decoded.payload));
      break;
    }
    case MessageType::kInputState: {
      if (!std::holds_alternative<protocol::InputStatePayload>(
              decoded.payload)) {
        logger_.get().Warn("Malformed input state from ", peer.endpoint_key);
        return;
      }
      HandleInputState(peer,
                       std::get<protocol::InputStatePayload>(decoded.payload),
                       decoded.header);
      break;
    }
    case MessageType::kClientCommand: {
      if (!std::holds_alternative<protocol::CommandPayload>(decoded.payload)) {
        logger_.get().Warn("Malformed client command from ", peer.endpoint_key);
        return;
      }
      HandleClientCommand(peer,
                          std::get<protocol::CommandPayload>(decoded.payload),
                          decoded.header);
      break;
    }
    case MessageType::kHello:
      logger_.get().Debug("Hello from ", peer.endpoint_key,
                          " ignored (connectionless)");
      break;
    case MessageType::kPong:
      logger_.get().Debug("Ignoring connectionless packet (type ",
                          static_cast<int>(type), ") from ", peer.endpoint_key);
      break;
    case MessageType::kServerCommand:
    case MessageType::kWorldSnapshot:
    case MessageType::kSpawnEntity:
    case MessageType::kDestroyEntity:
    case MessageType::kPlayerDied:
      logger_.get().Warn("Unexpected server-bound packet type ",
                         static_cast<int>(type), " from ", peer.endpoint_key);
      break;
    case MessageType::kInvalid:
      logger_.get().Warn("Received unexpected packet type ",
                         static_cast<int>(type), " from ", peer.endpoint_key);
      break;
    default:
      logger_.get().Debug("Ignoring non-join packet (type ",
                          static_cast<int>(type), ") from ", peer.endpoint_key);
      break;
  }
}

void ServerRuntime::HandlePing(PeerConnection& peer,
                               const protocol::PingPayload& ping) {
  protocol::PongPayload pong{};
  pong.client_time_ms = ping.client_time_ms;
  pong.server_time_ms = NowMilliseconds();

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type = static_cast<std::uint8_t>(MessageType::kPong);
  packet.header.flags = 0;
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = pong.server_time_ms;
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = pong;
  SendPacket(peer, packet);
}

void ServerRuntime::HandleInputState(
    PeerConnection& peer, const protocol::InputStatePayload& input_state,
    const protocol::Header& header) {
  if (peer.player_id == 0) {
    logger_.get().Warn("Received input state from unjoined peer ",
                       peer.endpoint_key);
    return;
  }
  logger_.get().Trace(
      "InputState from player ", peer.player_id,
      " command_count=", static_cast<int>(input_state.command_count));
  for (std::uint8_t i = 0; i < input_state.command_count; ++i) {
    const auto& command = input_state.commands[i];
    logger_.get().Trace("  Command ", i, ": seq=", command.input_sequence,
                        " buttons=", static_cast<int>(command.buttons),
                        " analog_x=", command.analog_x,
                        " analog_y=", command.analog_y,
                        " client_time_ms=", command.client_time_ms);
  }
  game_instance_.OnPlayerInput(peer.player_id, input_state, header);
}

void ServerRuntime::HandleClientCommand(PeerConnection& peer,
                                        const protocol::CommandPayload& command,
                                        const protocol::Header& header) {
  (void)header;

  if (peer.player_id == 0) {
    logger_.get().Warn("Received client command from unjoined peer ",
                       peer.endpoint_key);
    return;
  }
  logger_.get().Trace("ClientCommand from player ", peer.player_id,
                      " command_id=", command.command_id,
                      " data_size=", command.payload.size());
}

void ServerRuntime::ProcessJoin(PeerConnection& peer,
                                const protocol::JoinRequestPayload& request) {
  const std::string& endpoint_key = peer.endpoint_key;

  logger_.get().Debug("Join request from ", endpoint_key, " player ",
                      request.player_name, " room ", request.room_code);

  if (request.client_version != protocol::kProtocolVersion) {
    logger_.get().Warn("Rejecting join from ", endpoint_key,
                       " due to version mismatch");
    SendReject(peer, protocol::JoinRejectReason::kVersionMismatch,
               "Protocol version mismatch");
    return;
  }

  if (!config_.room_code.empty() && request.room_code != config_.room_code) {
    logger_.get().Warn("Rejecting join from ", endpoint_key, " invalid room ",
                       request.room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room unavailable");
    return;
  }

  if (peer.state == PeerState::kJoined && peer.player_id != 0) {
    logger_.get().Debug("Reusing existing player id ", peer.player_id, " for ",
                        endpoint_key);
    SendAccept(peer);
    return;
  }
  const std::size_t joined_count = CountJoinedPlayers();
  if (joined_count >= config_.max_players) {
    logger_.get().Warn("Rejecting join from ", endpoint_key,
                       " because lobby is full");
    SendReject(peer, protocol::JoinRejectReason::kServerFull, "Server is full");
    return;
  }

  peer.player_id = next_player_id_++;
  peer.state = PeerState::kJoined;
  peer.last_activity_ms = NowMilliseconds();
  players_[peer.player_id] = peer.endpoint_key;
  game_instance_.OnPlayerJoined(peer.player_id, request.player_name);
  logger_.get().Info("Accepted join from ", endpoint_key, " assigned id ",
                     peer.player_id);
  SendAccept(peer);
}

void ServerRuntime::SendServerCommand(PeerConnection& peer,
                                      std::uint16_t command_id,
                                      std::string_view payload) {
  protocol::CommandPayload command{};
  command.command_id = command_id;
  command.payload.assign(payload.begin(), payload.end());

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kServerCommand);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.header.timestamp_ms = NowMilliseconds();
  packet.payload = command;
  SendPacket(peer, packet);
}

void ServerRuntime::BroadcastServerCommand(std::uint16_t command_id,
                                           std::string_view payload) {
  for (auto& [_, peer] : peers_) {
    if (peer.state != PeerState::kJoined || peer.player_id == 0) {
      continue;
    }
    SendServerCommand(peer, command_id, payload);
  }
}

void ServerRuntime::SendAccept(PeerConnection& peer) {
  protocol::JoinAcceptPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.player_id = peer.player_id;
  payload.max_players = static_cast<std::uint8_t>(config_.max_players);
  payload.tick_rate = static_cast<std::uint8_t>(config_.tick_rate);
  payload.seed = rng_();

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kJoinAccept);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);

  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = payload;
  SendPacket(peer, packet);
}

void ServerRuntime::SendReject(PeerConnection& peer,
                               protocol::JoinRejectReason reason,
                               std::string_view message) {
  protocol::JoinRejectPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.reason = reason;
  payload.message.assign(message.begin(), message.end());

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kJoinReject);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = payload;
  SendPacket(peer, packet);
}

void ServerRuntime::SendPacket(PeerConnection& peer,
                               const protocol::Packet& packet) {
  protocol::Packet packet_to_send = packet;
  const auto type =
      static_cast<MessageType>(packet_to_send.header.message_type);
  const bool reliable_by_policy = protocol::IsReliable(type);
  if (reliable_by_policy) {
    packet_to_send.header.flags |=
        static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  }

  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet_to_send, buffer)) {
    logger_.get().Error("Failed to encode packet for ", peer.endpoint_key);
    return;
  }

  const bool is_reliable =
      reliable_by_policy ||
      ((packet_to_send.header.flags &
        static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable)) !=
       0);

  const auto send_result = transport_.Send(peer.endpoint, buffer);
  if (send_result.error) {
    logger_.get().Error("Send error to ", peer.endpoint_key, ": ",
                        send_result.error.message());
  }

  if (is_reliable && peer.reliable_queue) {
    peer.reliable_queue->AddSentPacket(packet_to_send.header.sequence,
                                       buffer.storage(), NowMilliseconds());
  }
}

void ServerRuntime::ProcessPeerAcks(PeerConnection& peer,
                                    const protocol::Header& header) {
  if (peer.reliable_queue == nullptr) {
    return;
  }
  peer.reliable_queue->OnAckReceived(header.ack, header.ack_bits);
}

void ServerRuntime::ProcessReliableResends() {
  const std::uint32_t now_ms = NowMilliseconds();

  for (auto& [_, peer] : peers_) {
    if (!peer.reliable_queue) {
      continue;
    }
    std::vector<protocol::PendingPacket> to_resend;
    peer.reliable_queue->CollectPacketsToResend(now_ms, to_resend);
    for (const auto& pending : to_resend) {
      const auto send_result = transport_.Send(
          peer.endpoint, pending.bytes.data(), pending.bytes.size());
      if (send_result.error) {
        logger_.get().Warn("Resend error to ", peer.endpoint_key, ": ",
                           send_result.error.message());
        peer.reliable_queue->MarkSendFailed(pending.sequence, now_ms);
      }
    }
  }
}

std::optional<std::reference_wrapper<PeerConnection>> ServerRuntime::FindPeer(
    const engine::net::Endpoint& from) {
  const auto endpoint_key = EndpointKey(from);
  const auto it = peers_.find(endpoint_key);
  if (it != peers_.end()) {
    return std::ref(it->second);
  }
  return std::nullopt;
}

std::size_t ServerRuntime::CountJoinedPlayers() const {
  std::size_t count = 0;
  for (const auto& [key, peer] : peers_) {
    if (peer.state == PeerState::kJoined) {
      ++count;
    }
  }
  return count;
}

PeerConnection& ServerRuntime::GetOrCreatePeer(
    const engine::net::Endpoint& endpoint) {
  const auto key = EndpointKey(endpoint);
  auto it = peers_.find(key);
  if (it != peers_.end()) {
    if (!it->second.reliable_queue) {
      it->second.reliable_queue = std::make_unique<protocol::ReliableQueue>(
          kReliableResendTimeoutMs, kReliableQueueMaxPending);
    }
    return it->second;
  }

  PeerConnection peer{};
  peer.endpoint_key = key;
  peer.endpoint = endpoint;
  peer.state = PeerState::kConnecting;
  peer.last_activity_ms = NowMilliseconds();
  peer.reliable_queue = std::make_unique<protocol::ReliableQueue>(
      kReliableResendTimeoutMs, kReliableQueueMaxPending);
  auto [inserted_it, inserted] = peers_.emplace(key, std::move(peer));
  (void)inserted;
  return inserted_it->second;
}

void ServerRuntime::CheckPeerTimeouts() {
  const std::uint32_t now_ms = NowMilliseconds();

  for (auto it = peers_.begin(); it != peers_.end();) {
    PeerConnection& peer = it->second;
    const std::uint32_t inactive_ms =
        now_ms >= peer.last_activity_ms
            ? now_ms - peer.last_activity_ms
            : (std::numeric_limits<std::uint32_t>::max() -
               peer.last_activity_ms) +
                  1u + now_ms;
    if (inactive_ms > kPeerTimeoutMs) {
      logger_.get().Info("Timing out peer ", peer.endpoint_key, " after ",
                         inactive_ms, " ms of inactivity");
      const std::uint32_t player_id = peer.player_id;
      const std::string endpoint_key = peer.endpoint_key;
      if (player_id != 0) {
        players_.erase(player_id);
        game_instance_.OnPlayerLeft(player_id);
      }
      it = peers_.erase(it);
      continue;
    }
    ++it;
  }
}

std::optional<std::reference_wrapper<PeerConnection>>
ServerRuntime::FindPeerByPlayerId(std::uint32_t player_id) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    return std::nullopt;
  }
  auto peer_it = peers_.find(it->second);
  if (peer_it == peers_.end()) {
    return std::nullopt;
  }
  return std::ref(peer_it->second);
}

void ServerRuntime::RemovePeer(PeerConnection& peer) {
  const std::string endpoint_key = peer.endpoint_key;
  const std::uint32_t player_id = peer.player_id;

  logger_.get().Info("Removing peer ", endpoint_key, " player id ", player_id);
  if (player_id != 0) {
    players_.erase(player_id);
    game_instance_.OnPlayerLeft(player_id);
  }
  peers_.erase(endpoint_key);
}

void ServerRuntime::BroadcastWorldSnapshot() {
  protocol::WorldSnapshotPayload snapshot =
      game_instance_.BuildWorldSnapshot(next_snapshot_id_++, server_tick_);
  snapshot_history_.AddSnapshot(snapshot);

  for (auto& [_, peer] : peers_) {
    if (peer.state != PeerState::kJoined || peer.player_id == 0) {
      continue;
    }
    protocol::Packet packet{};
    packet.header.version = protocol::kProtocolVersion;
    packet.header.message_type =
        static_cast<std::uint8_t>(MessageType::kWorldSnapshot);
    packet.header.flags = 0;
    packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
    packet.header.timestamp_ms = NowMilliseconds();
    peer.sequence_tracker.FillAckFields(packet.header);
    packet.payload = snapshot;
    SendPacket(peer, packet);
  }
}

}  // namespace server
