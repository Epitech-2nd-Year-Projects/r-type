#include "server_runtime.h"

#include <array>
#include <chrono>
#include <limits>
#include <string>
#include <thread>
#include <utility>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"

namespace server {
constexpr std::uint32_t kPeerTimeoutMs = 15'000;

namespace {

using protocol::message_type::MessageType;

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted;
}

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

}  // namespace

ServerRuntime::ServerRuntime(ServerConfig config)
    : socket_(engine::net::UdpSocket::Protocol::kIpv4),
      config_(std::move(config)),
      frame_timer_(static_cast<float>(config_.tick_rate)),
      rng_(config_.seed) {
  logger_ = &engine::util::Logger::Default();
}

std::error_code ServerRuntime::Start() {
  ConfigureLogging();

  const auto bind_endpoint = engine::net::Endpoint::AnyIpv4(config_.port);
  if (auto open_error = socket_.open(engine::net::UdpSocket::Protocol::kIpv4);
      open_error) {
    return open_error;
  }
  if (auto bind_error = socket_.bind(bind_endpoint); bind_error) {
    return bind_error;
  }

  logger_->Info("Server listening on ", bind_endpoint.address(), ":",
                bind_endpoint.port());
  const std::string room_label =
      config_.room_code.empty() ? std::string{"<any>"} : config_.room_code;
  logger_->Info("Room ", room_label, " max players ", config_.max_players,
                " tickrate ", config_.tick_rate, " seed ", config_.seed);
  return {};
}

void ServerRuntime::Run() {
  std::array<std::uint8_t, 2048> buffer{};

  while (true) {
    const auto delta = frame_timer_.tick();
    const auto recv_result = socket_.receive_from(buffer.data(), buffer.size());

    if (recv_result.error) {
      if (IsTransientError(recv_result.error)) {
        CheckPeerTimeouts();
        TickRateSleep(delta);
        continue;
      }
      logger_->Error("Receive error: ", recv_result.error.message());
      break;
    }

    if (recv_result.bytes_transferred > 0) {
      engine::net::PacketBuffer packet_buffer(buffer.data(),
                                              recv_result.bytes_transferred);
      HandlePacket(std::move(packet_buffer), recv_result.remote_endpoint);
    }
    CheckPeerTimeouts();
    TickRateSleep(delta);
  }
}

void ServerRuntime::ConfigureLogging() {
  logger_->SetName("server");
  logger_->SetLevel(config_.log_level);
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

  if (!protocol::DecodePacket(packet, decoded, &error)) {
    logger_->Warn("Dropped packet from ", EndpointKey(from), " (",
                  protocol::DecodeErrorToString(error), ")");
    return;
  }

  PeerConnection& peer = GetOrCreatePeer(from);
  peer.last_activity_ms = NowMilliseconds();
  peer.sequence_tracker.OnRemoteSequenceReceived(decoded.header.sequence);

  const auto type = static_cast<MessageType>(decoded.header.message_type);

  switch (type) {
    case MessageType::kJoinRequest: {
      const auto* request =
          std::get_if<protocol::JoinRequestPayload>(&decoded.payload);
      if (request == nullptr) {
        logger_->Warn("Malformed join request from ", peer.endpoint_key);
        return;
      }
      ProcessJoin(peer, *request);
      break;
    }
    case MessageType::kPing: {
      const auto* ping = std::get_if<protocol::PingPayload>(&decoded.payload);
      if (ping == nullptr) {
        logger_->Warn("Malformed ping from ", peer.endpoint_key);
        return;
      }
      HandlePing(peer, *ping);
      break;
    }

    default:
      logger_->Debug("Ignoring non-join packet (type ", static_cast<int>(type),
                     ") from ", peer.endpoint_key);
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
  peer.sequence_tracker.FillAckFields(&packet.header);
  packet.payload = pong;
  SendPacket(peer, packet);
}

void ServerRuntime::ProcessJoin(PeerConnection& peer,
                                const protocol::JoinRequestPayload& request) {
  const std::string& endpoint_key = peer.endpoint_key;

  logger_->Debug("Join request from ", endpoint_key, " player ",
                 request.player_name, " room ", request.room_code);

  if (request.client_version != protocol::kProtocolVersion) {
    logger_->Warn("Rejecting join from ", endpoint_key,
                  " due to version mismatch");
    SendReject(peer, protocol::JoinRejectReason::kVersionMismatch,
               "Protocol version mismatch");
    return;
  }

  if (!config_.room_code.empty() && request.room_code != config_.room_code) {
    logger_->Warn("Rejecting join from ", endpoint_key, " invalid room ",
                  request.room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room unavailable");
    return;
  }

  if (peer.state == PeerState::kJoined && peer.player_id != 0) {
    logger_->Debug("Reusing existing player id ", peer.player_id, " for ",
                   endpoint_key);
    SendAccept(peer);
    return;
  }
  const std::size_t joined_count = CountJoinedPlayers();
  if (joined_count >= config_.max_players) {
    logger_->Warn("Rejecting join from ", endpoint_key,
                  " because lobby is full");
    SendReject(peer, protocol::JoinRejectReason::kServerFull, "Server is full");
    return;
  }

  peer.player_id = next_player_id_++;
  peer.state = PeerState::kJoined;
  peer.last_activity_ms = NowMilliseconds();
  players_[peer.player_id] = peer.endpoint_key;
  logger_->Info("Accepted join from ", endpoint_key, " assigned id ",
                peer.player_id);
  SendAccept(peer);
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
  peer.sequence_tracker.FillAckFields(&packet.header);
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
  peer.sequence_tracker.FillAckFields(&packet.header);
  packet.payload = payload;
  SendPacket(peer, packet);
}

void ServerRuntime::SendPacket(PeerConnection& peer,
                               const protocol::Packet& packet) {
  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet, buffer)) {
    logger_->Error("Failed to encode packet for ", peer.endpoint_key);
    return;
  }
  const auto send_result =
      socket_.send_to(buffer.data(), buffer.size(), peer.endpoint);
  if (send_result.error) {
    logger_->Error("Send error to ", peer.endpoint_key, ": ",
                   send_result.error.message());
  }
}

PeerConnection* ServerRuntime::FindPeer(const engine::net::Endpoint& from) {
  const auto endpoint_key = EndpointKey(from);
  const auto it = peers_.find(endpoint_key);
  if (it != peers_.end()) {
    return &it->second;
  }
  return nullptr;
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
    return it->second;
  }

  PeerConnection peer{};
  peer.endpoint_key = key;
  peer.endpoint = endpoint;
  peer.state = PeerState::kConnecting;
  peer.last_activity_ms = NowMilliseconds();
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
      logger_->Info("Timing out peer ", peer.endpoint_key, " after ",
                    inactive_ms, " ms of inactivity");
      RemovePeer(peer);
      it = peers_.begin();
      continue;
    }
    ++it;
  }
}

PeerConnection* ServerRuntime::GetPeerByPlayerId(std::uint32_t player_id) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    return nullptr;
  }
  auto peer_it = peers_.find(it->second);
  if (peer_it == peers_.end()) {
    return nullptr;
  }
  return &peer_it->second;
}

void ServerRuntime::RemovePeer(PeerConnection& peer) {
  logger_->Info("Removing peer ", peer.endpoint_key, " player id ",
                peer.player_id);

  if (peer.player_id != 0) {
    players_.erase(peer.player_id);
  }
  peer.state = PeerState::kDisconnected;
  peers_.erase(peer.endpoint_key);
}

}  // namespace server
