#include "ecs/player_prediction_system.h"

#include <algorithm>
#include <cmath>

#include "game_logic/constants.h"

namespace client::ecs {

namespace {

float Clamp(float value, float min_value, float max_value) {
  return std::max(min_value, std::min(value, max_value));
}

}  // namespace

PlayerPredictionSystem::PlayerPredictionSystem(engine::ecs::Registry& registry)
    : registry_(registry) {
  RegisterComponents();
}

void PlayerPredictionSystem::RegisterComponents() {
  registry_.RegisterComponent<PositionComponent>();
  registry_.RegisterComponent<VelocityComponent>();
  registry_.RegisterComponent<SnapshotInterpolationComponent>();
  registry_.RegisterComponent<NetworkedEntityComponent>();
  registry_.RegisterComponent<LocalPlayerTag>();
}

std::optional<engine::ecs::EntityId> PlayerPredictionSystem::FindLocalPlayer()
    const {
  const auto& locals = registry_.GetComponents<LocalPlayerTag>();
  for (std::size_t i = 0; i < locals.size(); ++i) {
    if (locals[i].has_value()) {
      return registry_.EntityFromIndex(i);
    }
  }
  return std::nullopt;
}

engine::math::Vector2f PlayerPredictionSystem::BuildVelocity(
    const ActionState& action_state) const {
  engine::math::Vector2f velocity{0.0f, 0.0f};

  if (action_state.move_left && !action_state.move_right) {
    velocity.x = -game_logic::kPlayerMoveSpeed;
  } else if (action_state.move_right && !action_state.move_left) {
    velocity.x = game_logic::kPlayerMoveSpeed;
  }

  if (action_state.move_up && !action_state.move_down) {
    velocity.y = -game_logic::kPlayerMoveSpeed;
  } else if (action_state.move_down && !action_state.move_up) {
    velocity.y = game_logic::kPlayerMoveSpeed;
  }

  return velocity;
}

void PlayerPredictionSystem::Update(engine::time::TimeDelta dt,
                                    const ActionState& action_state) {
  auto& positions = registry_.GetComponents<PositionComponent>();
  const auto& networks = registry_.GetComponents<NetworkedEntityComponent>();

  const auto entity = FindLocalPlayer();
  if (!entity.has_value()) {
    has_prediction_ = false;
    last_snapshot_id_ = 0;
    last_local_entity_.reset();
    return;
  }
  if (!last_local_entity_.has_value() ||
      last_local_entity_.value() != entity.value()) {
    has_prediction_ = false;
    last_snapshot_id_ = 0;
    last_local_entity_ = entity;
  }

  const std::size_t index = static_cast<std::size_t>(entity.value());
  if (index >= positions.size() || !positions[index].has_value()) {
    has_prediction_ = false;
    last_snapshot_id_ = 0;
    return;
  }

  auto& position = positions[index].value();
  const engine::math::Vector2f authoritative = position.position;

  std::uint32_t snapshot_id = 0;
  if (index < networks.size() && networks[index].has_value()) {
    snapshot_id = networks[index]->last_snapshot;
  }

  const bool new_snapshot = snapshot_id > last_snapshot_id_;
  if (new_snapshot) {
    last_snapshot_id_ = snapshot_id;
  }
  if (!has_prediction_) {
    predicted_position_ = authoritative;
    has_prediction_ = true;
  }

  const float dt_seconds = dt.as_seconds();

  if (new_snapshot) {
    const engine::math::Vector2f delta = authoritative - predicted_position_;
    const float distance = delta.Length();

    if (distance > snap_distance_) {
      predicted_position_ = authoritative;
    } else {
      const float blend =
          std::min(1.0f, correction_rate_ * std::max(0.0f, dt_seconds));
      predicted_position_ = engine::math::Vector2f::Lerp(predicted_position_,
                                                         authoritative, blend);
    }
  }

  const engine::math::Vector2f predicted_velocity = BuildVelocity(action_state);
  predicted_position_ += predicted_velocity * dt_seconds;
  predicted_position_.x =
      Clamp(predicted_position_.x, 0.0f, game_logic::kGameWidth);
  predicted_position_.y =
      Clamp(predicted_position_.y, 0.0f, game_logic::kGameHeight);

  position.render_position = predicted_position_;
}

}  // namespace client::ecs
