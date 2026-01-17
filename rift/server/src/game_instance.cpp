#include "game_instance.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace server {

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t seed,
                           std::uint32_t max_players,
                           engine::util::Logger &logger)
    : rng_(seed),
      logic_(std::make_unique<rift::GameInstance>(room_id, max_players)),
      logger_(logger) {}

void GameInstance::OnPlayerJoined(std::uint32_t player_id,
                                  std::string_view player_name) {
  logger_.Info("[GameInstance] Player joined: ", player_id);

  PlayerState state{};
  state.player_id = player_id;
  state.connected = true;
  state.is_ready = false;
  players_[player_id] = state;
  if (logic_) {
    const std::string name = player_name.empty()
                                 ? "Player_" + std::to_string(player_id)
                                 : std::string{player_name};
    logic_->OnPlayerJoin(player_id, name);
  }
}

void GameInstance::OnPlayerLeft(std::uint32_t player_id) {
  logger_.Info("[GameInstance] Player left: ", player_id);
  players_.erase(player_id);
  if (logic_) {
    logic_->OnPlayerLeave(player_id);
  }
  if (players_.empty()) {
    phase_ = Phase::kLobby;
  }
}

void GameInstance::OnPlayerInput(std::uint32_t player_id,
                                 const protocol::InputStatePayload &payload,
                                 const protocol::Header &header) {
  (void)header;
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    logger_.Warn("[GameInstance] Input from unknown player: ", player_id);
    return;
  }

  PlayerState &state = it->second;

  if (payload.command_count == 0 ||
      payload.command_count > protocol::kMaxInputSequenceHistory) {
    logger_.Warn(
        "[GameInstance] Invalid InputStatePayload from player ", player_id,
        " (command_count=", static_cast<int>(payload.command_count), ")");
    return;
  }

  std::optional<std::reference_wrapper<const protocol::InputCommand>> newest;

  for (std::uint8_t i = 0; i < payload.command_count; ++i) {
    const protocol::InputCommand &cmd = payload.commands[i];

    if (cmd.input_sequence <= state.last_applied_sequence) {
      continue;
    }
    if (!newest || cmd.input_sequence > newest->get().input_sequence) {
      newest = std::cref(cmd);
    }
  }

  if (!newest) {
    logger_.Trace(
        "[GameInstance] All input commands already applied for player ",
        player_id);
    return;
  }

  auto emit_edge = [&](protocol::InputButton flag,
                       rift::GameInstance::InputEventType pressed_evt,
                       rift::GameInstance::InputEventType released_evt) {
    const bool was_pressed =
        (state.last_buttons & static_cast<std::uint8_t>(flag)) != 0;
    const bool is_set =
        (newest->get().buttons & static_cast<std::uint8_t>(flag)) != 0;
    if (!logic_) return;
    if (!was_pressed && is_set) {
      logic_->OnPlayerInput(player_id, pressed_evt);
    } else if (was_pressed && !is_set) {
      logic_->OnPlayerInput(player_id, released_evt);
    }
  };

  // Movement mapped to fight axis (Left/Right)
  emit_edge(protocol::InputButton::kInputLeft,
            rift::GameInstance::InputEventType::kMoveLeftPressed,
            rift::GameInstance::InputEventType::kMoveLeftReleased);
  emit_edge(protocol::InputButton::kInputRight,
            rift::GameInstance::InputEventType::kMoveRightPressed,
            rift::GameInstance::InputEventType::kMoveRightReleased);

  // Up = Block, Down = Dodge
  emit_edge(protocol::InputButton::kInputUp,
            rift::GameInstance::InputEventType::kBlockPressed,
            rift::GameInstance::InputEventType::kBlockReleased);
  emit_edge(protocol::InputButton::kInputDown,
            rift::GameInstance::InputEventType::kDodgePressed,
            rift::GameInstance::InputEventType::kDodgeReleased);

  // Fire = Light Attack, BigFire = Heavy Attack
  emit_edge(protocol::InputButton::kInputFire,
            rift::GameInstance::InputEventType::kLightAttackPressed,
            rift::GameInstance::InputEventType::kLightAttackReleased);
  emit_edge(protocol::InputButton::kInputBigFire,
            rift::GameInstance::InputEventType::kHeavyAttackPressed,
            rift::GameInstance::InputEventType::kHeavyAttackReleased);

  state.last_command = newest->get();
  state.last_buttons = newest->get().buttons;
  state.last_applied_sequence = newest->get().input_sequence;
  state.last_input_client_time_ms = newest->get().client_time_ms;
  state.last_input_server_time_ms = static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
  logger_.Trace("[GameInstance] Updated input for player ", player_id,
                " seq=", newest->get().input_sequence,
                " buttons=", static_cast<int>(newest->get().buttons),
                " ax=", newest->get().analog_x, " ay=", newest->get().analog_y);
}

std::optional<GameInstance::ReadyEvent> GameInstance::OnClientCommand(
    std::uint32_t player_id, const protocol::CommandPayload &command) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    logger_.Warn("[GameInstance] Command from unknown player: ", player_id);
    return std::nullopt;
  }

  const auto cmd_type = static_cast<protocol::CommandType>(command.command_id);
  bool target_ready = it->second.is_ready;

  switch (cmd_type) {
    case protocol::CommandType::kSetReady:
      target_ready = true;
      break;
    case protocol::CommandType::kUnready:
      target_ready = false;
      break;
    default:
      logger_.Debug("[GameInstance] Unknown command from player ", player_id,
                    ": ", command.command_id);
      return std::nullopt;
  }

  if (it->second.is_ready == target_ready) {
    return std::nullopt;
  }
  it->second.is_ready = target_ready;

  ReadyEvent evt{};
  evt.player_id = player_id;
  evt.is_ready = target_ready;

  if (phase_ == Phase::kLobby && CheckStartCondition()) {
    phase_ = Phase::kPlaying;
    if (logic_) {
      logic_->Start();
    }
    evt.game_started = true;
    logger_.Info("[GameInstance] Both players ready, starting match");
  } else {
    evt.game_started = false;
  }

  logger_.Info("[GameInstance] Player ", player_id,
               target_ready ? " ready" : " unready");
  return evt;
}

void GameInstance::Update(const engine::time::TimeDelta &delta) {
  if (phase_ == Phase::kLobby) {
    return;
  }
  if (logic_) {
    logic_->Update(delta);
  }
}

std::uint16_t GameInstance::ResolveEntityType(
    std::optional<std::reference_wrapper<const engine::ecs::TagComponent>> tag,
    std::optional<
        std::reference_wrapper<const rift::components::FighterComponent>>
        fighter) const {
  if (fighter.has_value()) {
    return 1;  // Fighter entity
  }
  if (!tag.has_value()) {
    return 0;
  }
  if (tag->get().tag == "Hitbox") return 2;
  if (tag->get().tag == "Effect") return 3;
  return 0;
}

protocol::WorldSnapshotPayload GameInstance::BuildWorldSnapshot(
    std::uint32_t snapshot_id, std::uint32_t server_tick) {
  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = snapshot_id;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.server_tick = server_tick;
  snapshot.current_wave = logic_ ? logic_->State().round_number : 0;
  snapshot.round_timer_ms = logic_ ? logic_->State().round_timer_ms : 0;

  auto &registry = World();
  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto &velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto &tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto &fighters = registry.GetComponents<rift::components::FighterComponent>();
  auto &healths = registry.GetComponents<rift::components::HealthComponent>();

  const std::size_t count = positions.size();
  snapshot.deltas.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value()) {
      continue;
    }
    const auto &pos = positions[i].value().position;
    const auto vel_opt =
        (i < velocities.size() && velocities[i].has_value())
            ? std::optional<engine::math::Vector2f>(velocities[i]->velocity)
            : std::nullopt;
    const auto tag_opt =
        (i < tags.size() && tags[i].has_value())
            ? std::optional<
                  std::reference_wrapper<const engine::ecs::TagComponent>>(
                  tags[i].value())
            : std::nullopt;
    const auto fighter_opt =
        (i < fighters.size() && fighters[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const rift::components::FighterComponent>>(
                  fighters[i].value())
            : std::nullopt;
    const auto health_opt =
        (i < healths.size() && healths[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const rift::components::HealthComponent>>(
                  healths[i].value())
            : std::nullopt;

    protocol::EntityNetState state{};
    state.entity_id = static_cast<std::uint32_t>(i);
    state.type = ResolveEntityType(tag_opt, fighter_opt);
    state.x = static_cast<std::int16_t>(std::lround(pos.x));
    state.y = static_cast<std::int16_t>(std::lround(pos.y));

    if (vel_opt.has_value()) {
      state.vx = static_cast<std::int16_t>(std::lround(vel_opt->x));
      state.vy = static_cast<std::int16_t>(std::lround(vel_opt->y));
    }
    if (health_opt.has_value()) {
      state.hp = static_cast<std::uint8_t>(
          std::min<std::uint32_t>(health_opt->get().current_health, 255u));
      state.flags = health_opt->get().invulnerable ? 1u : 0u;
    } else {
      state.hp = 0;
      state.flags = 0;
    }
    if (fighter_opt.has_value()) {
      auto it = players_.find(fighter_opt->get().player_id);
      if (it != players_.end() && it->second.is_ready) {
        state.flags |= 2u;  // Ready flag
      }
      if (!fighter_opt->get().facing_right) {
        state.flags |= 4u;  // Facing left flag
      }
      state.score = fighter_opt->get().rounds_won;
      state.lives = 1;
      state.player_id = fighter_opt->get().player_id;
    }

    protocol::EntityDelta delta{};
    delta.op = protocol::EntityDeltaOp::kCreate;
    delta.entity_id = state.entity_id;
    delta.state = state;
    snapshot.deltas.push_back(delta);
  }
  return snapshot;
}

engine::ecs::Registry &GameInstance::World() { return logic_->World(); }

const engine::ecs::Registry &GameInstance::World() const {
  return logic_->World();
}

rift::GameInstance &GameInstance::Logic() { return *logic_; }

const rift::GameInstance &GameInstance::Logic() const { return *logic_; }

bool GameInstance::CheckStartCondition() const {
  if (players_.size() != 2) {
    return false;
  }
  for (const auto &[_, state] : players_) {
    if (!state.is_ready) {
      return false;
    }
  }
  return true;
}

}  // namespace server
