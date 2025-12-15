#include "server_runtime.h"

#include <algorithm>
#include <asio/post.hpp>
#include <asio/thread_pool.hpp>
#include <atomic>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iomanip>
#include <latch>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "protocol/command.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/reliability_policy.h"
#include "protocol/world_snapshot.h"

namespace server {
constexpr std::uint32_t kReliableResendTimeoutMs = 250;
constexpr std::size_t kReliableQueueMaxPending = 64;
constexpr std::uint32_t kDecodeMetricsLogIntervalMs = 10'000;
constexpr std::uint32_t kServerDiagnosticsLogIntervalMs = 10'000;
constexpr float kTickHealthWarningThreshold = 0.9f;

namespace {

using protocol::message_type::MessageType;

std::atomic_bool g_shutdown_requested{false};
std::atomic_bool g_dump_metrics_requested{false};
std::atomic_bool g_dump_sessions_requested{false};
std::atomic_bool g_reload_config_requested{false};
std::atomic_bool g_force_shutdown_requested{false};

void SignalHandler(int) { g_shutdown_requested.store(true); }
void MetricsSignalHandler(int) { g_dump_metrics_requested.store(true); }
void SessionsSignalHandler(int) { g_dump_sessions_requested.store(true); }
void ReloadSignalHandler(int) { g_reload_config_requested.store(true); }
void ForceShutdownHandler(int) {
  g_force_shutdown_requested.store(true);
  g_shutdown_requested.store(true);
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

std::uint32_t ElapsedMilliseconds(std::uint32_t from, std::uint32_t to) {
  return to >= from
             ? to - from
             : (std::numeric_limits<std::uint32_t>::max() - from) + 1u + to;
}

void InstallSignalHandlers() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
#ifdef SIGUSR1
  std::signal(SIGUSR1, MetricsSignalHandler);
#endif
#ifdef SIGUSR2
  std::signal(SIGUSR2, SessionsSignalHandler);
#endif
#ifdef SIGHUP
  std::signal(SIGHUP, ReloadSignalHandler);
#endif
#ifdef SIGQUIT
  std::signal(SIGQUIT, ForceShutdownHandler);
#endif
}

bool IsValidRoomCode(std::string_view room_code) {
  return room_code.size() <= protocol::kMaxRoomCodeLength;
}

bool IsPrivateRoomCode(std::string_view room_code) {
  if (room_code.size() != 4) {
    return false;
  }
  return std::all_of(room_code.begin(), room_code.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

bool IsValidPlayerName(std::string_view player_name) {
  return player_name.size() <= protocol::kMaxPlayerNameLength;
}

const char* ToString(PeerState state) {
  switch (state) {
    case PeerState::kConnecting:
      return "connecting";
    case PeerState::kJoined:
      return "joined";
    case PeerState::kDisconnected:
      return "disconnected";
  }
  return "unknown";
}

std::size_t ComputeWorkerCount() {
  const auto hw = std::thread::hardware_concurrency();
  return hw == 0 ? 1u : static_cast<std::size_t>(hw);
}

std::optional<std::string> GeneratePrivateCode(
    std::mt19937& rng, const std::unordered_map<std::string, Room>& rooms) {
  std::vector<std::string> available_codes;
  available_codes.reserve(10'000);
  for (int value = 0; value <= 9999; ++value) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << value;
    const std::string code = oss.str();
    if (rooms.find(code) == rooms.end()) {
      available_codes.push_back(code);
    }
  }
  if (available_codes.empty()) {
    return std::nullopt;
  }
  std::uniform_int_distribution<std::size_t> dist(0,
                                                  available_codes.size() - 1);
  return available_codes[dist(rng)];
}

std::string GeneratePublicCode(
    std::mt19937& rng, const std::unordered_map<std::string, Room>& rooms) {
  std::uniform_int_distribution<int> dist(10000, 99999);
  for (int attempt = 0; attempt < 128; ++attempt) {
    const int value = dist(rng);
    std::string code = "room-" + std::to_string(value);
    if (rooms.find(code) == rooms.end()) {
      return code;
    }
  }
  return "room-" + std::to_string(rng());
}

}  // namespace

ServerRuntime::ServerRuntime(ServerConfig config)
    : config_(std::move(config)),
      logger_(engine::util::Logger::Default()),
      frame_timer_(static_cast<float>(config_.tick_rate)),
      worker_count_(ComputeWorkerCount()),
      worker_pool_(worker_count_),
      rng_(config_.seed),
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

  logger_.Info("Server listening on ", transport_.local_endpoint().address());
  logger_.Info("Max players ", config_.max_players, " tickrate ",
               config_.tick_rate, " timeout_ms ", config_.peer_timeout_ms,
               " room_idle_ms ", config_.room_idle_timeout_ms, " seed ",
               config_.seed);
  logger_.Info("Worker threads ", worker_count_);
  return {};
}

void ServerRuntime::Run() { RunMainLoop(); }

void ServerRuntime::RunMainLoop() {
  running_ = true;
  accumulator_ = engine::time::TimeDelta::zero();
  const auto start_ms = NowMilliseconds();
  last_decode_metrics_log_ms_ = start_ms;
  last_server_stats_log_ms_ = start_ms;
  last_tick_health_sample_ms_ = start_ms;
  last_tick_health_sample_tick_ = server_tick_;
  frame_time_accumulator_ms_ = 0.0;
  frame_time_samples_ = 0;

  while (running_ && !g_shutdown_requested.load()) {
    const auto delta = frame_timer_.tick();
    frame_time_accumulator_ms_ += static_cast<double>(delta.as_milliseconds());
    ++frame_time_samples_;
    accumulator_ += delta;
    PollNetwork();
    if (!running_) break;
    ProcessReliableResends();

    while (running_ && accumulator_ >= fixed_delta_) {
      PollNetwork();
      if (!running_) break;
      ProcessReliableResends();
      UpdateRoomsParallel(fixed_delta_);
      ++server_tick_;
      BroadcastWorldSnapshots();
      accumulator_ -= fixed_delta_;
    }
    CheckPeerTimeouts();
    PruneOrphanedSessions();
    MaybeLogDecodeMetrics();
    MaybeLogServerStats();
    if (g_force_shutdown_requested.exchange(false)) {
      logger_.Warn("Force shutdown requested");
      running_ = false;
      break;
    }
    if (g_dump_sessions_requested.exchange(false)) {
      DumpSessions();
    }
    if (g_reload_config_requested.exchange(false)) {
      ReloadConfiguration();
    }
    TickRateSleep(delta);
  }
  LogDecodeMetricsSummary(true);
  running_ = false;
  transport_.Stop();
}

void ServerRuntime::ConfigureLogging() {
  logger_.SetName("server");
  logger_.SetLevel(config_.log_level);
}

void ServerRuntime::PollNetwork() {
  const auto poll = transport_.PollNetwork();
  for (auto& datagram : poll.datagrams) {
    HandlePacket(std::move(datagram.payload), datagram.from);
  }
  if (poll.error) {
    logger_.Error("Receive error: ", poll.error.message());
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

void ServerRuntime::MaybeLogDecodeMetrics() {
  const auto now_ms = NowMilliseconds();
  const bool force_dump = g_dump_metrics_requested.exchange(false);
  const auto elapsed_ms =
      ElapsedMilliseconds(last_decode_metrics_log_ms_, now_ms);
  if (!force_dump && elapsed_ms < kDecodeMetricsLogIntervalMs) {
    return;
  }
  last_decode_metrics_log_ms_ = now_ms;
  LogDecodeMetricsSummary(force_dump);
}

void ServerRuntime::MaybeLogServerStats() {
  const auto now_ms = NowMilliseconds();
  const auto elapsed_ms =
      ElapsedMilliseconds(last_server_stats_log_ms_, now_ms);
  if (elapsed_ms < kServerDiagnosticsLogIntervalMs) {
    return;
  }

  std::size_t joined_peers = 0;
  std::size_t connecting_peers = 0;
  std::size_t disconnected_peers = 0;
  for (const auto& [_, peer] : peers_) {
    switch (peer.state) {
      case PeerState::kJoined:
        ++joined_peers;
        break;
      case PeerState::kConnecting:
        ++connecting_peers;
        break;
      case PeerState::kDisconnected:
        ++disconnected_peers;
        break;
    }
  }

  const auto health_window_ms =
      ElapsedMilliseconds(last_tick_health_sample_ms_, now_ms);
  const auto ticks_sampled = server_tick_ >= last_tick_health_sample_tick_
                                 ? server_tick_ - last_tick_health_sample_tick_
                                 : 0;
  const double elapsed_seconds =
      health_window_ms > 0 ? static_cast<double>(health_window_ms) / 1000.0
                           : 0.0;
  const double actual_tick_rate =
      elapsed_seconds > 0.0
          ? static_cast<double>(ticks_sampled) / elapsed_seconds
          : 0.0;
  const double target_tick_rate =
      static_cast<double>(config_.tick_rate > 0 ? config_.tick_rate : 1);
  const double health_ratio =
      target_tick_rate > 0.0 ? actual_tick_rate / target_tick_rate : 0.0;
  const bool tick_healthy = health_ratio >= kTickHealthWarningThreshold;
  const double avg_frame_ms = frame_time_samples_ > 0
                                  ? frame_time_accumulator_ms_ /
                                        static_cast<double>(frame_time_samples_)
                                  : 0.0;

  logger_.Info("Diagnostics peers total=", peers_.size(),
               " joined=", joined_peers, " connecting=", connecting_peers,
               " disconnected=", disconnected_peers,
               " players=", players_.size(), " rooms=", rooms_.size(),
               " tickrate target=", config_.tick_rate,
               " actual=", actual_tick_rate, " avg_frame_ms=", avg_frame_ms,
               " backlog_ms=", accumulator_.as_milliseconds(),
               " health=", tick_healthy ? "ok" : "degraded");

  for (const auto& [room_code, room] : rooms_) {
    logger_.Info("Room ", room_code, " [",
                 (room.IsPrivate() ? "private" : "public"), "] players ",
                 room.PlayerCount(), "/", room.MaxPlayers());
  }

  last_server_stats_log_ms_ = now_ms;
  last_tick_health_sample_ms_ = now_ms;
  last_tick_health_sample_tick_ = server_tick_;
  frame_time_accumulator_ms_ = 0.0;
  frame_time_samples_ = 0;
}

void ServerRuntime::UpdateRoomsParallel(const engine::time::TimeDelta& delta) {
  if (rooms_.empty()) {
    return;
  }

  std::vector<std::reference_wrapper<Room>> rooms_snapshot;
  rooms_snapshot.reserve(rooms_.size());
  for (auto& [_, room] : rooms_) {
    rooms_snapshot.push_back(std::ref(room));
  }

  if (worker_count_ <= 1 || rooms_snapshot.size() == 1) {
    for (auto& room_ref : rooms_snapshot) {
      room_ref.get().Update(delta);
    }
    return;
  }

  std::latch latch(static_cast<std::ptrdiff_t>(rooms_snapshot.size()));
  for (auto room_ref : rooms_snapshot) {
    asio::post(worker_pool_, [room_ref, delta, &latch, this]() {
      try {
        room_ref.get().Update(delta);
      } catch (const std::exception& ex) {
        logger_.Error("Room update failed: ", ex.what());
      } catch (...) {
        logger_.Error("Room update failed with unknown error");
      }
      latch.count_down();
    });
  }
  latch.wait();
}

void ServerRuntime::DumpSessions() {
  logger_.Info("Session dump peers=", peers_.size(),
               " players=", players_.size(), " rooms=", rooms_.size(),
               " tick=", server_tick_);

  for (const auto& [room_code, room] : rooms_) {
    logger_.Info("Room ", room_code, " id=", room.Id(), " players ",
                 room.PlayerCount(), "/", room.MaxPlayers(),
                 " privacy=", (room.IsPrivate() ? "private" : "public"),
                 " seed=", room.Seed());
  }

  for (const auto& [endpoint, peer] : peers_) {
    logger_.Info("Peer ", endpoint, " state=", ToString(peer.state),
                 " player_id=", peer.player_id, " room=", peer.room_code);
  }
}

void ServerRuntime::ReloadConfiguration() {
  const char* env_log_level = std::getenv("RTYPE_SERVER_LOG_LEVEL");
  if (env_log_level != nullptr) {
    config_.log_level =
        engine::util::ParseLogLevel(env_log_level, config_.log_level);
  }
  ConfigureLogging();
  logger_.Info("Configuration reloaded log_level=",
               engine::util::ToString(config_.log_level));
}

void ServerRuntime::LogDecodeMetricsSummary(bool force) {
  const auto& m = decode_metrics_;
  if (!force && m.total_packets == 0) {
    return;
  }
  logger_.Info("DecodeMetrics total=", m.total_packets,
               " rejected=", m.rejected_packets,
               " errors[unknown_message_type=", m.unknown_message_type,
               " version_mismatch=", m.version_mismatch,
               " unexpected_end_of_buffer=", m.unexpected_end_of_buffer,
               " invalid_header=", m.invalid_header,
               " invalid_payload=", m.invalid_payload,
               " invalid_snapshot_id=", m.invalid_snapshot_id, "]");
}

void ServerRuntime::HandlePacket(engine::net::PacketBuffer packet,
                                 const engine::net::Endpoint& from) {
  protocol::Packet decoded{};
  protocol::DecodeError error{protocol::DecodeError::kOk};

  if (!protocol::DecodePacket(packet, decoded, error)) {
    protocol::UpdateDecodeMetrics(decode_metrics_, error);
    logger_.Warn("Dropped packet from ", EndpointKey(from), " (",
                 protocol::DecodeErrorToString(error),
                 ") total=", decode_metrics_.total_packets,
                 " rejected=", decode_metrics_.rejected_packets);
    return;
  }
  protocol::UpdateDecodeMetrics(decode_metrics_, protocol::DecodeError::kOk);
  PeerConnection& peer = GetOrCreatePeer(from);
  peer.last_seen_ms = NowMilliseconds();
  peer.sequence_tracker.OnRemoteSequenceReceived(decoded.header.sequence);
  ProcessPeerAcks(peer, decoded.header);

  const auto type = static_cast<MessageType>(decoded.header.message_type);

  switch (type) {
    case MessageType::kJoinRequest: {
      if (!std::holds_alternative<protocol::JoinRequestPayload>(
              decoded.payload)) {
        logger_.Warn("Malformed join request from ", peer.endpoint_key);
        return;
      }
      ProcessJoin(peer,
                  std::get<protocol::JoinRequestPayload>(decoded.payload));
      break;
    }
    case MessageType::kRoomListRequest: {
      HandleRoomListRequest(peer);
      break;
    }
    case MessageType::kCreateRoomRequest: {
      if (!std::holds_alternative<protocol::CreateRoomRequestPayload>(
              decoded.payload)) {
        logger_.Warn("Malformed room creation request from ",
                     peer.endpoint_key);
        return;
      }
      HandleCreateRoomRequest(
          peer, std::get<protocol::CreateRoomRequestPayload>(decoded.payload));
      break;
    }
    case MessageType::kPing: {
      if (!std::holds_alternative<protocol::PingPayload>(decoded.payload)) {
        logger_.Warn("Malformed ping from ", peer.endpoint_key);
        return;
      }
      HandlePing(peer, std::get<protocol::PingPayload>(decoded.payload));
      break;
    }
    case MessageType::kInputState: {
      if (!std::holds_alternative<protocol::InputStatePayload>(
              decoded.payload)) {
        logger_.Warn("Malformed input state from ", peer.endpoint_key);
        return;
      }
      HandleInputState(peer,
                       std::get<protocol::InputStatePayload>(decoded.payload),
                       decoded.header);
      break;
    }
    case MessageType::kClientCommand: {
      if (!std::holds_alternative<protocol::CommandPayload>(decoded.payload)) {
        logger_.Warn("Malformed client command from ", peer.endpoint_key);
        return;
      }
      HandleClientCommand(peer,
                          std::get<protocol::CommandPayload>(decoded.payload),
                          decoded.header);
      break;
    }
    case MessageType::kHello:
      logger_.Debug("Hello from ", peer.endpoint_key,
                    " ignored (connectionless)");
      break;
    case MessageType::kPong:
      logger_.Debug("Ignoring connectionless packet (type ",
                    static_cast<int>(type), ") from ", peer.endpoint_key);
      break;
    case MessageType::kServerCommand:
    case MessageType::kWorldSnapshot:
    case MessageType::kSpawnEntity:
    case MessageType::kDestroyEntity:
    case MessageType::kPlayerDied:
      logger_.Warn("Unexpected server-bound packet type ",
                   static_cast<int>(type), " from ", peer.endpoint_key);
      break;
    case MessageType::kInvalid:
      logger_.Warn("Received unexpected packet type ", static_cast<int>(type),
                   " from ", peer.endpoint_key);
      break;
    default:
      logger_.Debug("Ignoring non-join packet (type ", static_cast<int>(type),
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
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = pong;
  SendPacket(peer, packet);
}

void ServerRuntime::HandleInputState(
    PeerConnection& peer, const protocol::InputStatePayload& input_state,
    const protocol::Header& header) {
  if (peer.player_id == 0) {
    logger_.Warn("Received input state from unjoined peer ", peer.endpoint_key);
    return;
  }
  logger_.Trace("InputState from player ", peer.player_id,
                " command_count=", static_cast<int>(input_state.command_count));
  for (std::uint8_t i = 0; i < input_state.command_count; ++i) {
    const auto& command = input_state.commands[i];
    logger_.Trace("  Command ", i, ": seq=", command.input_sequence,
                  " buttons=", static_cast<int>(command.buttons),
                  " analog_x=", command.analog_x,
                  " analog_y=", command.analog_y,
                  " client_time_ms=", command.client_time_ms);
  }
  auto room = FindRoom(peer.room_code);
  if (!room.has_value()) {
    logger_.Warn("No game instance for peer ", peer.endpoint_key);
    return;
  }
  room->get().HandleInput(peer.player_id, input_state, header);
  room->get().MarkActive(NowMilliseconds());
}

void ServerRuntime::HandleClientCommand(PeerConnection& peer,
                                        const protocol::CommandPayload& command,
                                        const protocol::Header& header) {
  (void)header;

  if (peer.player_id == 0) {
    logger_.Warn("Received client command from unjoined peer ",
                 peer.endpoint_key);
    return;
  }
  auto room = FindRoom(peer.room_code);
  if (!room.has_value()) {
    logger_.Warn("Client command for unknown room from ", peer.endpoint_key);
    return;
  }

  if (command.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice)) {
    logger_.Info("Peer requested disconnect ", peer.endpoint_key);
    DisconnectPeer(peer, "client disconnect", /*notify_client=*/false);
    peers_.erase(peer.endpoint_key);
    return;
  }

  logger_.Trace("ClientCommand from player ", peer.player_id,
                " command_id=", command.command_id,
                " data_size=", command.payload.size());

  const auto ready_event =
      room->get().HandleClientCommand(peer.player_id, command);
  room->get().MarkActive(NowMilliseconds());
  if (!ready_event.has_value()) {
    return;
  }

  const std::uint16_t ready_command_id = static_cast<std::uint16_t>(
      ready_event->is_ready ? protocol::CommandType::kSetReady
                            : protocol::CommandType::kUnready);
  const std::string payload = std::to_string(ready_event->player_id);

  const auto& players = room->get().Players();
  for (std::uint32_t player_id : players) {
    auto peer_ref = FindPeerByPlayerId(player_id);
    if (!peer_ref.has_value()) {
      continue;
    }
    PeerConnection& target = peer_ref->get();
    if (target.state != PeerState::kJoined || target.player_id == 0 ||
        target.room_code != peer.room_code) {
      continue;
    }
    SendServerCommand(target, ready_command_id, payload);
    if (ready_event->game_started) {
      SendServerCommand(
          target, static_cast<std::uint16_t>(protocol::CommandType::kStartGame),
          "");
    }
  }
}

void ServerRuntime::ProcessJoin(PeerConnection& peer,
                                const protocol::JoinRequestPayload& request) {
  const std::string& endpoint_key = peer.endpoint_key;

  logger_.Debug("Join request from ", endpoint_key, " player ",
                request.player_name, " room ", request.room_code);

  if (!IsValidPlayerName(request.player_name)) {
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " invalid player name length");
    SendReject(peer, protocol::JoinRejectReason::kUnknown,
               "Player name too long");
    return;
  }

  if (request.client_version != protocol::kProtocolVersion) {
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " due to version mismatch");
    SendReject(peer, protocol::JoinRejectReason::kVersionMismatch,
               "Protocol version mismatch");
    return;
  }

  std::string room_code = request.room_code;
  if (room_code.empty()) {
    logger_.Warn("Rejecting join from ", endpoint_key, " missing room code");
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room code required");
    return;
  }
  if (room_code.empty() || !IsValidRoomCode(room_code)) {
    logger_.Warn("Rejecting join from ", endpoint_key, " invalid room code");
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room code too long");
    return;
  }

  auto room = FindRoom(room_code);
  if (!room.has_value()) {
    logger_.Warn("Rejecting join from ", endpoint_key, " unknown room ",
                 room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room not found");
    return;
  }

  if (room->get().IsPrivate()) {
    if (!IsPrivateRoomCode(request.room_password) ||
        request.room_password != room->get().Password()) {
      logger_.Warn("Rejecting join from ", endpoint_key,
                   " invalid password for room ", room_code);
      SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
                 "Invalid password");
      return;
    }
  }

  if (peer.state == PeerState::kJoined && peer.player_id != 0) {
    if (peer.room_code == room_code) {
      logger_.Debug("Reusing existing player id ", peer.player_id, " for ",
                    endpoint_key, " room ", room_code);
      SendAccept(peer, room_code);
      return;
    }
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " already joined in room ", peer.room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Already joined");
    return;
  }

  Room& room_ref = room->get();

  peer.player_id = next_player_id_++;
  peer.state = PeerState::kJoined;
  peer.room_code = room_code;
  peer.room_id = room_ref.Id();
  peer.last_seen_ms = NowMilliseconds();
  if (!JoinRoom(peer, room_ref, request.player_name)) {
    peer.player_id = 0;
    peer.state = PeerState::kConnecting;
    peer.room_code.clear();
    peer.room_id = 0;
    SendReject(peer, protocol::JoinRejectReason::kServerFull, "Room full");
    return;
  }
  logger_.Info("Accepted join from ", endpoint_key, " assigned id ",
               peer.player_id, " room ", room_code);
  SendAccept(peer, room_code);
}

void ServerRuntime::HandleRoomListRequest(PeerConnection& peer) {
  protocol::RoomListResponsePayload payload{};
  std::vector<const Room*> public_rooms;
  public_rooms.reserve(rooms_.size());
  for (const auto& [_, room] : rooms_) {
    if (room.IsPrivate()) {
      continue;
    }
    public_rooms.push_back(&room);
  }
  std::sort(public_rooms.begin(), public_rooms.end(),
            [](const Room* lhs, const Room* rhs) {
              return lhs->Name() < rhs->Name();
            });
  payload.rooms.reserve(std::min<std::size_t>(public_rooms.size(),
                                              protocol::kMaxRoomListEntries));
  for (const Room* room : public_rooms) {
    if (payload.rooms.size() >= protocol::kMaxRoomListEntries) {
      break;
    }
    payload.rooms.push_back(BuildRoomSummary(*room));
  }

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kRoomListResponse);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = std::move(payload);
  SendPacket(peer, packet);
}

void ServerRuntime::HandleCreateRoomRequest(
    PeerConnection& peer, const protocol::CreateRoomRequestPayload& request) {
  protocol::CreateRoomResponsePayload response{};

  std::uint16_t requested_capacity =
      request.max_players == 0 ? config_.max_players : request.max_players;
  requested_capacity = std::max<std::uint16_t>(1, requested_capacity);
  requested_capacity = std::min<std::uint16_t>(
      requested_capacity, std::numeric_limits<std::uint8_t>::max());

  if (request.room_name.empty()) {
    response.message = "Room name required";
  } else if (request.room_name.size() > protocol::kMaxRoomNameLength) {
    response.message = "Room name too long";
  }

  std::string room_code;
  std::string password;
  if (request.is_private) {
    if (!request.room_password.empty()) {
      if (!IsPrivateRoomCode(request.room_password)) {
        response.message = "Invalid password (must be 4 digits)";
      } else {
        password = request.room_password;
      }
    } else {
      auto generated = GeneratePrivateCode(rng_, rooms_);
      if (!generated.has_value()) {
        response.message = "Unable to allocate a private code";
      } else {
        password = *generated;
      }
    }
    if (response.message.empty()) {
      room_code = "priv-" + password;
    }
  } else if (room_code.empty()) {
    room_code = GeneratePublicCode(rng_, rooms_);
  }

  if (response.message.empty() && !IsValidRoomCode(room_code)) {
    response.message = "Room code too long";
  }
  if (response.message.empty() && rooms_.find(room_code) != rooms_.end()) {
    response.message = "Room already exists";
  }

  if (response.message.empty()) {
    Room& room = CreateRoom(room_code, request.room_name, request.is_private,
                            password, requested_capacity);
    response.success = true;
    response.message = "Room created";
    response.room = BuildRoomSummary(room);
    response.room_password = password;
    logger_.Info("Created room ", room_code,
                 request.is_private ? " (private)" : " (public)", " capacity ",
                 requested_capacity);
  }

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kCreateRoomResponse);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = std::move(response);
  SendPacket(peer, packet);
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

void ServerRuntime::SendAccept(PeerConnection& peer,
                               const std::string& room_code) {
  auto room = FindRoomConst(room_code);
  protocol::JoinAcceptPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.player_id = peer.player_id;
  payload.max_players = static_cast<std::uint8_t>(
      room ? room->get().MaxPlayers() : config_.max_players);
  payload.tick_rate = static_cast<std::uint8_t>(config_.tick_rate);
  payload.seed = room ? room->get().Seed() : rng_();

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
    logger_.Error("Failed to encode packet for ", peer.endpoint_key);
    return;
  }

  const bool is_reliable =
      reliable_by_policy ||
      ((packet_to_send.header.flags &
        static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable)) !=
       0);

  const auto send_result = transport_.Send(peer.endpoint, buffer);
  if (send_result.error) {
    logger_.Error("Send error to ", peer.endpoint_key, ": ",
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
        logger_.Warn("Resend error to ", peer.endpoint_key, ": ",
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

std::optional<std::reference_wrapper<Room>> ServerRuntime::FindRoom(
    const std::string& room_code) {
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return std::nullopt;
  }
  return std::ref(it->second);
}

std::optional<std::reference_wrapper<const Room>> ServerRuntime::FindRoomConst(
    const std::string& room_code) const {
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return std::nullopt;
  }
  return std::cref(it->second);
}

void ServerRuntime::EnsureDefaultRoom() {
  if (config_.default_room_code.empty()) {
    return;
  }
  if (rooms_.find(config_.default_room_code) != rooms_.end()) {
    return;
  }
  const std::uint16_t capacity = static_cast<std::uint16_t>(
      std::max<std::uint16_t>(1, config_.max_players));
  Room& room = CreateRoom(config_.default_room_code, config_.default_room_name,
                          /*is_private=*/false, "", capacity);
  logger_.Info("Bootstrapped room ", room.Code(), " capacity ",
               room.MaxPlayers());
}

Room& ServerRuntime::CreateRoom(const std::string& room_code,
                                const std::string& room_name, bool is_private,
                                std::string password,
                                std::uint16_t max_players) {
  const std::uint32_t room_id = next_room_id_++;
  const std::uint32_t seed = rng_();
  auto [inserted, _] = rooms_.emplace(
      room_code, Room{room_code, room_name, room_id, max_players, is_private,
                      std::move(password), seed, logger_});
  inserted->second.MarkActive(NowMilliseconds());
  return inserted->second;
}

protocol::RoomSummary ServerRuntime::BuildRoomSummary(const Room& room) const {
  protocol::RoomSummary summary{};
  summary.room_code = room.Code();
  summary.room_name = room.Name();
  summary.is_private = room.IsPrivate();
  summary.max_players = static_cast<std::uint8_t>(std::min<std::uint16_t>(
      room.MaxPlayers(), std::numeric_limits<std::uint8_t>::max()));
  summary.player_count = static_cast<std::uint8_t>(std::min<std::size_t>(
      room.PlayerCount(),
      static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())));
  return summary;
}

void ServerRuntime::CleanupRoomIfEmpty(const std::string& room_code,
                                       std::uint32_t now_ms) {
  if (room_code.empty()) {
    return;
  }
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return;
  }
  const std::uint32_t last_active = it->second.LastActiveMs();
  const std::uint32_t idle_timeout_ms = config_.room_idle_timeout_ms;
  const bool idle = last_active != 0 && now_ms >= last_active
                        ? now_ms - last_active >= idle_timeout_ms
                        : false;
  if (!it->second.IsEmpty() || !idle) {
    return;
  }
  rooms_.erase(it);
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
  peer.last_seen_ms = NowMilliseconds();
  peer.reliable_queue = std::make_unique<protocol::ReliableQueue>(
      kReliableResendTimeoutMs, kReliableQueueMaxPending);
  auto [inserted_it, inserted] = peers_.emplace(key, std::move(peer));
  (void)inserted;
  return inserted_it->second;
}

void ServerRuntime::DisconnectPeer(PeerConnection& peer,
                                   std::string_view reason,
                                   bool notify_client) {
  const auto now_ms = NowMilliseconds();
  logger_.Info("Disconnecting peer ", peer.endpoint_key, " player id ",
               peer.player_id, " reason ", reason);
  if (notify_client && peer.state == PeerState::kJoined) {
    SendServerCommand(
        peer,
        static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice),
        reason);
  }
  peer.state = PeerState::kDisconnected;
  std::string room_code = LeaveRoom(peer, now_ms);
  peer.player_id = 0;
  peer.room_code.clear();
  peer.room_id = 0;
  CleanupRoomIfEmpty(room_code, now_ms);
}

void ServerRuntime::CheckPeerTimeouts() {
  const std::uint32_t now_ms = NowMilliseconds();

  for (auto it = peers_.begin(); it != peers_.end();) {
    PeerConnection& peer = it->second;
    const std::uint32_t inactive_ms =
        now_ms >= peer.last_seen_ms
            ? now_ms - peer.last_seen_ms
            : (std::numeric_limits<std::uint32_t>::max() - peer.last_seen_ms) +
                  1u + now_ms;
    if (inactive_ms <= config_.peer_timeout_ms) {
      ++it;
      continue;
    }

    DisconnectPeer(peer, "timeout", peer.state == PeerState::kJoined);
    it = peers_.erase(it);
  }
}

void ServerRuntime::PruneOrphanedSessions() {
  const std::uint32_t now_ms = NowMilliseconds();
  std::vector<std::uint32_t> to_remove;
  to_remove.reserve(players_.size());

  for (const auto& [player_id, session] : players_) {
    auto peer_it = peers_.find(session.endpoint_key);
    const bool missing_peer = peer_it == peers_.end();
    const bool invalid_peer =
        !missing_peer && (peer_it->second.state != PeerState::kJoined ||
                          peer_it->second.player_id != player_id);
    if (missing_peer || invalid_peer) {
      to_remove.push_back(player_id);
    }
  }

  for (std::uint32_t player_id : to_remove) {
    auto session_it = players_.find(player_id);
    if (session_it == players_.end()) {
      continue;
    }
    const std::string room_code = session_it->second.room_code;
    players_.erase(session_it);
    if (auto room = FindRoom(room_code)) {
      if (room->get().RemovePlayer(player_id)) {
        room->get().MarkActive(now_ms);
      }
    }
    CleanupRoomIfEmpty(room_code, now_ms);
  }
}

std::optional<std::reference_wrapper<PeerConnection>>
ServerRuntime::FindPeerByPlayerId(std::uint32_t player_id) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    return std::nullopt;
  }
  auto peer_it = peers_.find(it->second.endpoint_key);
  if (peer_it == peers_.end()) {
    return std::nullopt;
  }
  return std::ref(peer_it->second);
}

void ServerRuntime::RemovePeer(PeerConnection& peer) {
  const std::string endpoint_key = peer.endpoint_key;
  DisconnectPeer(peer, "removed", peer.state == PeerState::kJoined);
  peers_.erase(endpoint_key);
}

bool ServerRuntime::JoinRoom(PeerConnection& peer, Room& room,
                             std::string_view player_name) {
  if (room.PlayerCount() >= room.MaxPlayers()) {
    return false;
  }
  if (!room.AddPlayer(peer.player_id, player_name)) {
    return false;
  }
  players_[peer.player_id] = PlayerSession{peer.endpoint_key, room.Code()};
  room.MarkActive(peer.last_seen_ms);
  return true;
}

std::string ServerRuntime::LeaveRoom(PeerConnection& peer,
                                     std::uint32_t now_ms) {
  std::string room_code = peer.room_code;
  if (peer.player_id == 0) {
    return room_code;
  }
  const auto session = players_.find(peer.player_id);
  if (session != players_.end()) {
    room_code = session->second.room_code;
    players_.erase(session);
  }
  if (auto room = FindRoom(room_code)) {
    if (room->get().RemovePlayer(peer.player_id)) {
      room->get().MarkActive(now_ms);
    }
  }
  return room_code;
}

void ServerRuntime::BroadcastWorldSnapshots() {
  std::vector<std::string> empty_rooms;
  const auto now_ms = NowMilliseconds();

  for (auto& [room_code, room] : rooms_) {
    if (room.IsEmpty()) {
      empty_rooms.push_back(room_code);
      continue;
    }

    protocol::WorldSnapshotPayload snapshot = room.BuildSnapshot(server_tick_);
    room.MarkActive(now_ms);

    for (std::uint32_t player_id : room.Players()) {
      auto peer_ref = FindPeerByPlayerId(player_id);
      if (!peer_ref.has_value()) {
        continue;
      }
      PeerConnection& peer = peer_ref->get();
      if (peer.state != PeerState::kJoined || peer.player_id == 0 ||
          peer.room_code != room_code) {
        continue;
      }
      protocol::Packet packet{};
      packet.header.version = protocol::kProtocolVersion;
      packet.header.message_type =
          static_cast<std::uint8_t>(MessageType::kWorldSnapshot);
      packet.header.flags = 0;
      packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
      packet.header.timestamp_ms = now_ms;
      peer.sequence_tracker.FillAckFields(packet.header);
      packet.payload = snapshot;
      SendPacket(peer, packet);
    }
  }

  for (const auto& room_code : empty_rooms) {
    CleanupRoomIfEmpty(room_code, now_ms);
  }
}

}  // namespace server
