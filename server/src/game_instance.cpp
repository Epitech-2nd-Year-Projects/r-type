#include "game_instance.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace server {

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t seed,
                           std::uint32_t max_players,
                           protocol::Difficulty difficulty,
                           engine::util::Logger &logger)
    : rng_(seed),
      logic_(std::make_unique<game_logic::GameInstance>(
          room_id, max_players,
          static_cast<game_logic::Difficulty>(difficulty))),
      logger_(logger),
      history_(1000),
      current_tick_(0) {}

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
    logger_.Info("[GameInstance] Calling Logic OnPlayerJoin");
    logic_->OnPlayerJoin(player_id, name);
    logger_.Info("[GameInstance] Logic OnPlayerJoin returned");
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
                       game_logic::GameInstance::InputEventType event_type,
                       game_logic::GameInstance::InputEventType released_evt) {
    const bool was_pressed =
        (state.last_buttons & static_cast<std::uint8_t>(flag)) != 0;
    const bool is_set =
        (newest->get().buttons & static_cast<std::uint8_t>(flag)) != 0;
    if (!logic_) return;
    std::optional<engine::math::Vector2f> spawn_pos = std::nullopt;
    float latency_s = 0.0f;

    if ((flag == protocol::InputButton::kInputFire ||
         flag == protocol::InputButton::kInputBigFire) &&
        !was_pressed && is_set) {
      std::uint32_t current_server_time = static_cast<std::uint32_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now().time_since_epoch())
              .count());
      std::uint32_t client_time = newest->get().client_time_ms;

      std::uint32_t delta = current_server_time - client_time;
      if (delta < 0x80000000u) {
        latency_s = static_cast<float>(delta) / 1000.0f;
        spawn_pos = history_.GetPlayerPositionAt(player_id, client_time);
      }
    }

    if (!was_pressed && is_set) {
      logic_->OnPlayerInput(player_id, event_type, spawn_pos, latency_s);
    } else if (was_pressed && !is_set) {
      logic_->OnPlayerInput(player_id, released_evt);
    }
  };
  emit_edge(protocol::InputButton::kInputUp,
            game_logic::GameInstance::InputEventType::kMoveUpPressed,
            game_logic::GameInstance::InputEventType::kMoveUpReleased);
  emit_edge(protocol::InputButton::kInputDown,
            game_logic::GameInstance::InputEventType::kMoveDownPressed,
            game_logic::GameInstance::InputEventType::kMoveDownReleased);
  emit_edge(protocol::InputButton::kInputLeft,
            game_logic::GameInstance::InputEventType::kMoveLeftPressed,
            game_logic::GameInstance::InputEventType::kMoveLeftReleased);
  emit_edge(protocol::InputButton::kInputRight,
            game_logic::GameInstance::InputEventType::kMoveRightPressed,
            game_logic::GameInstance::InputEventType::kMoveRightReleased);
  emit_edge(protocol::InputButton::kInputFire,
            game_logic::GameInstance::InputEventType::kBasicShootPressed,
            game_logic::GameInstance::InputEventType::kBasicShootReleased);
  emit_edge(protocol::InputButton::kInputBigFire,
            game_logic::GameInstance::InputEventType::kBigShootPressed,
            game_logic::GameInstance::InputEventType::kBigShootReleased);
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
    logger_.Info("[GameInstance] Lobby conditions met, starting game");
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

    auto current_time_ms = static_cast<std::uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    current_tick_++;

    auto &registry = logic_->World();
    auto &players =
        registry.GetComponents<game_logic::components::PlayerComponent>();
    auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();

    for (size_t i = 0; i < players.size() && i < positions.size(); ++i) {
      if (players[i].has_value() && positions[i].has_value()) {
        history_.RecordSnapshot(current_tick_, current_time_ms,
                                players[i]->player_id, positions[i]->position);
      }
    }
  }
}

std::uint16_t GameInstance::ResolveEntityType(
    std::optional<std::reference_wrapper<const engine::ecs::TagComponent>> tag,
    std::optional<
        std::reference_wrapper<const game_logic::components::PlayerComponent>>
        player,
    std::optional<
        std::reference_wrapper<const game_logic::components::PowerupComponent>>
        powerup,
    std::optional<std::reference_wrapper<
        const game_logic::components::EnemyTypeComponent>>
        enemy_type) const {
  if (player.has_value()) {
    return 1;
  }
  if (powerup.has_value()) {
    return 10 + static_cast<std::uint16_t>(powerup->get().type);
  }
  if (enemy_type.has_value()) {
    return enemy_type->get().type_code;
  }
  if (!tag.has_value()) {
    return 0;
  }
  if (tag->get().tag == "Enemy") return 2;
  if (tag->get().tag == "Missile") return 3;
  if (tag->get().tag == "Obstacle") return 4;
  if (tag->get().tag == "Powerup") return 5;
  return 0;
}

protocol::WorldSnapshotPayload GameInstance::BuildWorldSnapshot(
    std::uint32_t snapshot_id, std::uint32_t server_tick) {
  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = snapshot_id;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.server_tick = server_tick;
  snapshot.current_wave = logic_ ? logic_->State().current_wave : 0;

  auto &registry = World();
  auto &position = registry.GetComponents<engine::ecs::PositionComponent>();
  auto &velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto &tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto &players =
      registry.GetComponents<game_logic::components::PlayerComponent>();
  auto &healths =
      registry.GetComponents<game_logic::components::HealthComponent>();
  auto &powerups =
      registry.GetComponents<game_logic::components::PowerupComponent>();
  auto &enemy_types =
      registry.GetComponents<game_logic::components::EnemyTypeComponent>();

  const std::size_t count = position.size();
  snapshot.deltas.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    if (!position[i].has_value()) {
      continue;
    }
    const auto &pos = position[i].value().position;
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
    const auto player_opt =
        (i < players.size() && players[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const game_logic::components::PlayerComponent>>(
                  players[i].value())
            : std::nullopt;
    const auto health_opt =
        (i < healths.size() && healths[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const game_logic::components::HealthComponent>>(
                  healths[i].value())
            : std::nullopt;
    const auto powerup_opt =
        (i < powerups.size() && powerups[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const game_logic::components::PowerupComponent>>(
                  powerups[i].value())
            : std::nullopt;
    const auto enemy_type_opt =
        (i < enemy_types.size() && enemy_types[i].has_value())
            ? std::optional<std::reference_wrapper<
                  const game_logic::components::EnemyTypeComponent>>(
                  enemy_types[i].value())
            : std::nullopt;

    protocol::EntityNetState state{};
    state.entity_id = static_cast<std::uint32_t>(i);
    state.type =
        ResolveEntityType(tag_opt, player_opt, powerup_opt, enemy_type_opt);
    state.x = static_cast<std::int16_t>(std::lround(pos.x));
    state.y = static_cast<std::int16_t>(std::lround(pos.y));

    if (vel_opt.has_value()) {
      state.vx = static_cast<std::int16_t>(std::lround(vel_opt->x));
      state.vy = static_cast<std::int16_t>(std::lround(vel_opt->y));
    }
    if (health_opt.has_value()) {
      state.hp = health_opt->get().current_health;
      state.flags = health_opt->get().invulnerable ? 1u : 0u;
    } else {
      state.hp = 0;
      state.flags = 0;
    }
    if (player_opt.has_value()) {
      auto it = players_.find(player_opt->get().player_id);
      if (it != players_.end() && it->second.is_ready) {
        state.flags |= 2u;
      }
      state.score = player_opt->get().score;
      state.lives = static_cast<std::uint8_t>(
          std::min<std::uint32_t>(player_opt->get().lives, 255u));
      state.player_id = player_opt->get().player_id;
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

game_logic::GameInstance &GameInstance::Logic() { return *logic_; }

const game_logic::GameInstance &GameInstance::Logic() const { return *logic_; }

bool GameInstance::CheckStartCondition() const {
  if (players_.size() < 1) {
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