#include "game_instance.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace server {

GameInstance::GameInstance(std::uint32_t seed)
    : rng_(seed), logic_(std::make_unique<game_logic::GameInstance>(1u, 4u)) {
  logic_->Start();
}

void GameInstance::OnPlayerJoined(std::uint32_t player_id) {
  auto& logger = engine::util::Logger::Default();
  logger.Info("[GameInstance] Player joined: ", player_id);

  PlayerState state{};
  state.player_id = player_id;
  state.connected = true;

  players_[player_id] = state;
}

void GameInstance::OnPlayerLeft(std::uint32_t player_id) {
  auto& logger = engine::util::Logger::Default();
  logger.Info("[GameInstance] Player left: ", player_id);

  players_.erase(player_id);
}

void GameInstance::OnPlayerInput(std::uint32_t player_id,
                                 const protocol::InputStatePayload& payload,
                                 const protocol::Header& header) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    auto& logger = engine::util::Logger::Default();
    logger.Warn("[GameInstance] Input from unknown player: ", player_id);
    return;
  }

  PlayerState& state = it->second;

  if (payload.command_count == 0 ||
      payload.command_count > protocol::kMaxInputSequenceHistory) {
    auto& logger = engine::util::Logger::Default();
    logger.Warn(
        "[GameInstance] Invalid InputStatePayload from player ", player_id,
        " (command_count=", static_cast<int>(payload.command_count), ")");
    return;
  }

  const protocol::InputCommand* newest = nullptr;

  for (std::uint8_t i = 0; i < payload.command_count; ++i) {
    const protocol::InputCommand& cmd = payload.commands[i];

    if (cmd.input_sequence <= state.last_applied_sequence) {
      continue;
    }
    if (newest == nullptr || cmd.input_sequence > newest->input_sequence) {
      newest = &cmd;
    }
  }

  if (newest == nullptr) {
    auto& logger = engine::util::Logger::Default();
    logger.Trace(
        "[GameInstance] All input commands already applied for player ",
        player_id);
    return;
  }
  state.last_command = *newest;
  state.last_applied_sequence = newest->input_sequence;
  state.last_input_client_time_ms = newest->client_time_ms;
  state.last_input_server_time_ms = static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
  auto& logger = engine::util::Logger::Default();
  logger.Trace("[GameInstance] Updated input for player ", player_id,
               " seq=", newest->input_sequence,
               " buttons=", static_cast<int>(newest->buttons),
               " ax=", newest->analog_x, " ay=", newest->analog_y);
}

void GameInstance::Update(const engine::time::TimeDelta& delta) {
  if (logic_) {
    logic_->Update(delta);
  }
}

std::uint16_t GameInstance::ResolveEntityType(
    const engine::ecs::TagComponent* tag,
    const game_logic::components::PlayerComponent* player) const {
  if (player != nullptr) {
    return 1;
  }
  if (tag == nullptr) {
    return 0;
  }
  if (tag->tag == "Enemy") return 2;
  if (tag->tag == "Missile") return 3;
  if (tag->tag == "Obstacle") return 4;
  return 0;
}

protocol::WorldSnapshotPayload GameInstance::BuildWorldSnapshot(
    std::uint32_t snapshot_id, std::uint32_t server_tick) {
  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = snapshot_id;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.server_tick = server_tick;

  auto& registry = World();
  auto& position = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto& players =
      registry.GetComponents<game_logic::components::PlayerComponent>();
  auto& healths =
      registry.GetComponents<game_logic::components::HealthComponent>();

  const std::size_t count = position.size();
  snapshot.deltas.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    if (!position[i].has_value()) {
      continue;
    }
    const auto& pos = position[i].value().position;
    const auto* vel_opt = (i < velocities.size() && velocities[i].has_value())
                              ? &velocities[i].value().velocity
                              : nullptr;
    const auto* tag_opt =
        (i < tags.size() && tags[i].has_value()) ? &tags[i].value() : nullptr;
    const auto* player_opt = (i < players.size() && players[i].has_value())
                                 ? &players[i].value()
                                 : nullptr;
    const auto* health_opt = (i < healths.size() && healths[i].has_value())
                                 ? &healths[i].value()
                                 : nullptr;
    protocol::EntityNetState state{};
    state.entity_id = static_cast<std::uint32_t>(i);
    state.type = ResolveEntityType(tag_opt, player_opt);
    state.x = static_cast<std::int16_t>(std::lround(pos.x));
    state.y = static_cast<std::int16_t>(std::lround(pos.y));

    if (vel_opt != nullptr) {
      state.vx = static_cast<std::int16_t>(std::lround(vel_opt->x));
      state.vy = static_cast<std::int16_t>(std::lround(vel_opt->y));
    }
    if (health_opt != nullptr) {
      state.hp = static_cast<std::uint8_t>(
          std::min<std::uint32_t>(health_opt->current_health, 255u));
      state.flags = health_opt->invulnerable ? 1u : 0u;
    } else {
      state.hp = 0;
      state.flags = 0;
    }
    protocol::EntityDelta delta{};
    delta.op = protocol::EntityDeltaOp::kCreate;
    delta.entity_id = state.entity_id;
    delta.state = state;
    snapshot.deltas.push_back(delta);
  }
  return snapshot;
}

engine::ecs::Registry& GameInstance::World() { return logic_->World(); }

const engine::ecs::Registry& GameInstance::World() const {
  return logic_->World();
}

game_logic::GameInstance& GameInstance::Logic() { return *logic_; }

const game_logic::GameInstance& GameInstance::Logic() const { return *logic_; }

}  // namespace server
