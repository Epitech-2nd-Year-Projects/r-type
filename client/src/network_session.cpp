#include "network_session.h"

#include <chrono>
#include <thread>
#include <utility>

#include "audio_controller.h"
#include "ecs/animation_system.h"
#include "ecs/components.h"
#include "ecs/interpolation_system.h"
#include "ecs/world_state_system.h"
#include "engine/net/packet_buffer.h"
#include "engine/time/monotonic_time.h"
#include "lobby_service.h"
#include "logging.h"
#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "world_update_receiver.h"

namespace client {
namespace {

LobbyRetryPolicy BuildLobbyRetryPolicy(const ClientConfig& config) {
  return LobbyRetryPolicy{
      config.lobby_max_attempts,
      std::chrono::milliseconds(config.lobby_retry_delay_ms)};
}

}  // namespace

NetworkSession::NetworkSession(ClientConfig config)
    : config_(std::move(config)),
      transport_(std::make_shared<NetworkTransport>()),
      lobby_transport_(std::make_shared<NetworkTransport>()),
      join_flow_(config_.player_name, config_.room_code),
      lobby_service_(std::make_unique<LobbyService>(
          lobby_transport_, BuildLobbyRetryPolicy(config_))),
      world_registry_(std::make_unique<engine::ecs::Registry>()),
      world_state_system_(
          std::make_unique<ecs::WorldStateSystem>(*world_registry_)),
      animation_system_(
          std::make_unique<ecs::AnimationSystem>(*world_registry_)),
      interpolation_system_(
          std::make_unique<ecs::InterpolationSystem>(*world_registry_)) {
  join_flow_.ConfigureRetryPolicy(
      config_.join_max_attempts,
      std::chrono::milliseconds(config_.join_retry_delay_ms));
  world_update_receiver_.Configure(
      std::chrono::milliseconds(config_.ping_interval_ms),
      config_.network_queue_size);
  SetInterpolationConfig(config_.interpolation_delay_ms,
                         config_.max_extrapolation_ms);
}

NetworkSession::~NetworkSession() { Shutdown(); }

NetworkEvents NetworkSession::Update(engine::time::TimeDelta dt,
                                     AudioController& audio) {
  NetworkEvents events;

  if (lobby_service_) {
    lobby_service_->Update();
  }

  if (transport_) {
    join_flow_.Update(*transport_);
  }

  JoinState join_state = join_flow_.state();
  if (join_state == JoinState::kConnected &&
      !world_update_receiver_.running()) {
    if (!world_update_receiver_.Start(transport_)) {
      LogNetwork(engine::util::LogLevel::kError,
                 "Failed to start world update receiver");
      events.stop_requested = true;
      return events;
    }
  }

  if (world_update_receiver_.running()) {
    WorldUpdateMessage message;
    while (world_update_receiver_.TryPop(message)) {
      if (message.type == protocol::message_type::MessageType::kWorldSnapshot) {
        if (const auto snapshot =
                std::get_if<protocol::WorldSnapshotPayload>(&message.payload)) {
          const std::uint64_t receipt_ms = engine::time::NowMilliseconds();
          if (snapshot->current_wave != 0) {
            last_wave_ = snapshot->current_wave;
          }
          if (world_state_system_) {
            const auto local_player = join_flow_.player_id();
            const auto tick_rate_hz = join_flow_.server_tick_rate_hz();
            const auto clock_offset_ms =
                world_update_receiver_.LatestClockOffsetMs();
            std::optional<std::uint64_t> snapshot_time_ms;
            if (clock_offset_ms.has_value()) {
              const std::int64_t offset =
                  static_cast<std::int64_t>(clock_offset_ms.value());
              const std::int64_t server_stamp =
                  static_cast<std::int64_t>(message.header.timestamp_ms);
              // clock_offset_ms is server minus client, so subtract to convert.
              const std::int64_t client_time = server_stamp - offset;
              snapshot_time_ms = client_time > 0
                                     ? static_cast<std::uint64_t>(client_time)
                                     : 0u;
            }
            world_state_system_->ApplySnapshot(*snapshot, receipt_ms,
                                               local_player, tick_rate_hz,
                                               snapshot_time_ms);
          }

          if (world_registry_) {
            audio.OnSnapshotApplied(*world_registry_);
          }
        }
      }
      if (message.type == protocol::message_type::MessageType::kPlayerDied) {
        audio.OnPlayerDeath();
        if (std::holds_alternative<protocol::PlayerDiedPayload>(
                message.payload)) {
          const auto& died =
              std::get<protocol::PlayerDiedPayload>(message.payload);
          if (died.player_id == join_flow_.player_id() &&
              died.remaining_lives == 0) {
            events.game_over = BuildGameOverStats();
          }
        }
      }
      if (message.type == protocol::message_type::MessageType::kServerCommand) {
        if (const auto command =
                std::get_if<protocol::CommandPayload>(&message.payload)) {
          HandleServerCommand(*command, events);
        }
      }
      if (join_flow_.state() != JoinState::kConnected) {
        break;
      }
    }
  }

  MonitorConnection(events);

  if (interpolation_system_) {
    interpolation_system_->Update(dt);
  }
  if (animation_system_) {
    animation_system_->Update(dt);
  }

  UpdateLocalPlayerCache();

  join_state = join_flow_.state();
  if (join_state == JoinState::kConnected &&
      last_join_state_ != JoinState::kConnected) {
    events.connected = true;
  }
  if (join_state == JoinState::kRefused &&
      last_join_state_ != JoinState::kRefused) {
    events.connection_failed = std::string(join_flow_.status());
  }
  if (join_state == JoinState::kDisconnected &&
      last_join_state_ != JoinState::kDisconnected &&
      !events.disconnected.has_value()) {
    events.disconnected = std::string(join_flow_.status());
  }
  last_join_state_ = join_state;

  return events;
}

std::optional<std::string> NetworkSession::StartConnection() {
  if (!transport_) {
    return std::string("Network transport unavailable");
  }
  disconnect_notice_sent_ = false;
  const auto transport_error = transport_->Start(config_.host, config_.port);
  if (transport_error) {
    const std::string reason =
        std::string("Failed to start network transport: ") +
        transport_error.message();
    join_flow_.MarkDisconnected(reason);
    return reason;
  }
  join_flow_.Begin(*transport_);
  return std::nullopt;
}

void NetworkSession::Stop() { Disconnect(DisconnectMode::kNotifyServer); }

void NetworkSession::Disconnect(DisconnectMode mode) {
  const bool should_notify = mode == DisconnectMode::kNotifyServer &&
                             !disconnect_notice_sent_ && transport_ &&
                             transport_->running() &&
                             join_flow_.state() == JoinState::kConnected;
  if (should_notify) {
    protocol::CommandPayload disconnect{};
    disconnect.command_id =
        static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice);
    const bool queued = world_update_receiver_.EnqueueCommand(disconnect);
    if (!queued) {
      protocol::Packet packet{};
      packet.header.version = protocol::kProtocolVersion;
      packet.header.message_type = static_cast<std::uint8_t>(
          protocol::message_type::MessageType::kClientCommand);
      packet.header.flags = 0;
      packet.header.sequence = 0;
      packet.header.ack = 0;
      packet.header.ack_bits = 0;
      packet.header.timestamp_ms =
          static_cast<std::uint32_t>(engine::time::NowMilliseconds());
      packet.payload = disconnect;
      engine::net::PacketBuffer buffer;
      buffer.reserve(64);
      if (protocol::EncodePacket(packet, buffer)) {
        (void)transport_->Send(std::move(buffer));
      }
    }
    disconnect_notice_sent_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  world_update_receiver_.Stop();
  if (transport_) {
    transport_->Stop();
  }
  if (world_state_system_) {
    world_state_system_->Reset();
  }
  cached_local_score_.reset();
  last_wave_ = 1u;
}

void NetworkSession::Reset() {
  join_flow_.Reset();
  cached_local_score_.reset();
  last_wave_ = 1u;
  last_join_state_ = JoinState::kIdle;
  disconnect_notice_sent_ = false;
}

void NetworkSession::Shutdown() {
  Disconnect(DisconnectMode::kNotifyServer);
  if (lobby_service_) {
    lobby_service_->Disconnect();
  }
}

void NetworkSession::SetConnectionConfig(std::string host, int port,
                                         std::string player_name,
                                         std::string room_code,
                                         std::string room_password) {
  config_.host = std::move(host);
  config_.port = static_cast<std::uint16_t>(port);
  config_.player_name = std::move(player_name);
  config_.room_code = std::move(room_code);

  join_flow_ = JoinFlow(config_.player_name, config_.room_code);
  join_flow_.ConfigureRetryPolicy(
      config_.join_max_attempts,
      std::chrono::milliseconds(config_.join_retry_delay_ms));
  join_flow_.SetRoomPassword(std::move(room_password));
  disconnect_notice_sent_ = false;
}

void NetworkSession::RefreshRoomList(std::string host, std::uint16_t port) {
  if (!lobby_service_) {
    return;
  }
  if (!lobby_service_->Connect(std::move(host), port)) {
    return;
  }
  lobby_service_->RequestRoomList();
}

void NetworkSession::CreateRoom(std::string host, std::uint16_t port,
                                std::string room_name, bool is_private,
                                std::string room_password,
                                std::uint16_t max_players,
                                protocol::Difficulty difficulty) {
  if (!lobby_service_) {
    return;
  }
  if (!lobby_service_->Connect(std::move(host), port)) {
    return;
  }
  lobby_service_->RequestCreateRoom(std::move(room_name), is_private,
                                    std::move(room_password), max_players,
                                    difficulty);
}

const std::vector<protocol::RoomSummary>& NetworkSession::RoomDirectoryRooms()
    const {
  static const std::vector<protocol::RoomSummary> kEmpty;
  return lobby_service_ ? lobby_service_->rooms() : kEmpty;
}

std::string NetworkSession::RoomDirectoryStatus() const {
  return lobby_service_ ? lobby_service_->status() : "Lobby unavailable";
}

std::optional<protocol::CreateRoomResponsePayload>
NetworkSession::ConsumeLastRoomCreation() {
  if (!lobby_service_) {
    return std::nullopt;
  }
  return lobby_service_->ConsumeCreateResponse();
}

std::string_view NetworkSession::JoinStatus() const {
  return join_flow_.status();
}

std::optional<std::uint32_t> NetworkSession::LocalPlayerId() const {
  return join_flow_.player_id();
}

bool NetworkSession::TransportRunning() const {
  return transport_ && transport_->running();
}

bool NetworkSession::EnqueueCommand(const protocol::CommandPayload& payload) {
  return world_update_receiver_.EnqueueCommand(payload);
}

void NetworkSession::SetInterpolationConfig(
    std::uint32_t interpolation_delay_ms, std::uint32_t max_extrapolation_ms) {
  if (!interpolation_system_) {
    return;
  }
  interpolation_system_->SetInterpolationDelayMs(interpolation_delay_ms);
  interpolation_system_->SetMaxExtrapolationMs(max_extrapolation_ms);
}

engine::ecs::Registry& NetworkSession::World() { return *world_registry_; }

const engine::ecs::Registry& NetworkSession::World() const {
  return *world_registry_;
}

std::optional<std::uint32_t> NetworkSession::CurrentWave() const {
  return last_wave_;
}

std::optional<float> NetworkSession::LatestLatencyMs() const {
  return world_update_receiver_.LatestRttMs();
}

WorldUpdateReceiver& NetworkSession::UpdateReceiver() {
  return world_update_receiver_;
}

void NetworkSession::HandleServerCommand(
    const protocol::CommandPayload& payload, NetworkEvents& events) {
  if (payload.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice)) {
    const std::string reason =
        payload.payload.empty() ? "Disconnected by server" : payload.payload;
    LogNetwork(engine::util::LogLevel::kWarn, reason);
    if (const auto lost = HandleConnectionLost(reason)) {
      events.disconnected = *lost;
    }
  }
}

std::optional<std::string> NetworkSession::HandleConnectionLost(
    std::string_view reason) {
  if (join_flow_.state() == JoinState::kDisconnected) {
    return std::nullopt;
  }

  Disconnect(DisconnectMode::kSilent);
  join_flow_.MarkDisconnected(reason);
  return std::string(reason);
}

void NetworkSession::MonitorConnection(NetworkEvents& events) {
  if (join_flow_.state() != JoinState::kConnected) {
    return;
  }

  if (!transport_ || !transport_->running()) {
    if (const auto lost = HandleConnectionLost("Connection closed")) {
      events.disconnected = *lost;
    }
    return;
  }

  const auto last_receive = transport_->last_receive_ms();
  if (last_receive == 0) {
    return;
  }

  const auto now_ms = engine::time::NowMilliseconds();
  const auto silence_ms = now_ms >= last_receive ? now_ms - last_receive : 0;
  if (silence_ms > config_.timeout_ms) {
    if (const auto lost =
            HandleConnectionLost("Timed out waiting for server")) {
      events.disconnected = *lost;
    }
  }
}

void NetworkSession::UpdateLocalPlayerCache() {
  if (!world_registry_ || !join_flow_.player_id().has_value()) {
    return;
  }
  const auto local_id = join_flow_.player_id().value();
  const auto& net =
      world_registry_->GetComponents<ecs::NetworkedEntityComponent>();
  const auto& player_states =
      world_registry_->GetComponents<ecs::PlayerStateComponent>();
  for (std::size_t i = 0; i < net.size(); ++i) {
    if (!net[i].has_value()) {
      continue;
    }
    if (i >= player_states.size() || !player_states[i].has_value()) {
      continue;
    }
    if (player_states[i]->player_id != local_id) {
      continue;
    }
    cached_local_score_ = player_states[i]->score;
    return;
  }
}

GameOverStats NetworkSession::BuildGameOverStats() const {
  GameOverStats stats;
  if (join_flow_.player_id().has_value()) {
    const auto local_id = join_flow_.player_id().value();
    const auto& net =
        world_registry_
            ? world_registry_->GetComponents<ecs::NetworkedEntityComponent>()
            : engine::ecs::SparseArray<ecs::NetworkedEntityComponent>();
    const auto& player_states =
        world_registry_
            ? world_registry_->GetComponents<ecs::PlayerStateComponent>()
            : engine::ecs::SparseArray<ecs::PlayerStateComponent>();
    bool found_score = false;
    for (std::size_t i = 0; i < net.size(); ++i) {
      if (!net[i].has_value()) {
        continue;
      }
      if (i < player_states.size() && player_states[i].has_value() &&
          player_states[i]->player_id == local_id) {
        stats.score = player_states[i]->score;
        found_score = true;
        break;
      }
    }
    if (!found_score && cached_local_score_.has_value()) {
      stats.score = *cached_local_score_;
    }
  }
  if (last_wave_.has_value()) {
    stats.wave = *last_wave_;
  }
  return stats;
}

}  // namespace client

namespace client {
void NetworkSession::AttachDebugger(engine::debug::NetworkDebugger& debugger) {
  if (transport_) {
    transport_->AttachDebugger(debugger);
  }
  if (lobby_transport_) {
    lobby_transport_->AttachDebugger(debugger);
  }
}
}  // namespace client
