#include "network_session.h"

#include <chrono>
#include <thread>
#include <utility>

#include "ecs/components.h"
#include "ecs/world_state_system.h"
#include "engine/ecs/registry.h"
#include "engine/net/packet_buffer.h"
#include "engine/time/monotonic_time.h"
#include "logging.h"
#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "world_update_receiver.h"

namespace rift::client {

NetworkSession::NetworkSession(RiftConfig config)
    : config_(std::move(config)),
      transport_(std::make_shared<NetworkTransport>()),
      join_flow_(config_.player_name, ""),
      world_registry_(std::make_unique<engine::ecs::Registry>()) {
  join_flow_.ConfigureRetryPolicy(
      config_.join_max_attempts,
      std::chrono::milliseconds(config_.join_retry_delay_ms));
  world_update_receiver_.Configure(
      std::chrono::milliseconds(config_.ping_interval_ms), 256);

  world_registry_->RegisterComponent<ecs::NetworkedEntityComponent>();
  world_registry_->RegisterComponent<ecs::PositionComponent>();
  world_registry_->RegisterComponent<ecs::VelocityComponent>();
  world_registry_->RegisterComponent<ecs::FighterTag>();
  world_registry_->RegisterComponent<ecs::LocalFighterTag>();
  world_registry_->RegisterComponent<ecs::FighterStateComponent>();
  world_registry_->RegisterComponent<ecs::CombatDisplayComponent>();
  world_registry_->RegisterComponent<ecs::HealthBarComponent>();
  world_registry_->RegisterComponent<ecs::StaminaBarComponent>();
  world_registry_->RegisterComponent<ecs::FighterRenderComponent>();
  world_registry_->RegisterComponent<ecs::Fighter3DRenderComponent>();
  world_registry_->RegisterComponent<ecs::AnimationComponent>();
  world_registry_->RegisterComponent<ecs::SnapshotInterpolationComponent>();

  world_state_system_ =
      std::make_unique<ecs::WorldStateSystem>(*world_registry_);
}

NetworkSession::~NetworkSession() { Shutdown(); }

NetworkEvents NetworkSession::Update(engine::time::TimeDelta dt) {
  NetworkEvents events;

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
          const auto now_ms = engine::time::NowMilliseconds();
          world_state_system_->ApplySnapshot(*snapshot, LocalPlayerId(), now_ms);
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
}

void NetworkSession::Reset() {
  join_flow_.Reset();
  last_join_state_ = JoinState::kIdle;
  disconnect_notice_sent_ = false;
}

void NetworkSession::Shutdown() { Disconnect(DisconnectMode::kNotifyServer); }

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

engine::ecs::Registry& NetworkSession::World() { return *world_registry_; }

const engine::ecs::Registry& NetworkSession::World() const {
  return *world_registry_;
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
  if (payload.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kStartGame)) {
    events.match_started = true;
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

}  // namespace rift::client
