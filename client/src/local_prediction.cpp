#include "local_prediction.h"

#include <algorithm>

#include "ecs/archetype_registry.h"

namespace client {

namespace {

bool IsPlayerMatch(const ecs::NetworkedEntityComponent& net,
                   std::optional<std::uint32_t> player_id) {
  return player_id.has_value() && net.network_id == *player_id;
}

bool HasPlayerTag(const engine::ecs::Registry& registry,
                  engine::ecs::EntityId entity) {
  const auto& tags = registry.GetComponents<ecs::PlayerTag>();
  const std::size_t idx = static_cast<std::size_t>(entity);
  return idx < tags.size() && tags[idx].has_value();
}

}  // namespace

LocalPrediction::LocalPrediction(engine::ecs::Registry& registry,
                                 JoinFlow& join_flow)
    : registry_(registry), join_flow_(join_flow) {}

void LocalPrediction::Reset() {
  local_entity_.reset();
  reconciliation_offset_ = {0.0f, 0.0f};
  reconciliation_elapsed_ = 0.0f;
}

std::optional<engine::ecs::EntityId> LocalPrediction::ResolveLocalEntity() {
  if (local_entity_.has_value()) {
    const std::size_t idx = static_cast<std::size_t>(*local_entity_);
    const auto& net = registry_.GetComponents<ecs::NetworkedEntityComponent>();
    if (idx < net.size() && net[idx].has_value()) {
      return local_entity_;
    }
  }

  const auto player_id = join_flow_.player_id();
  const auto& net = registry_.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& archetypes = ecs::ArchetypeRegistry::Get();
  if (player_id.has_value()) {
    for (std::size_t i = 0; i < net.size(); ++i) {
      if (!net[i].has_value()) {
        continue;
      }
      const auto& comp = net[i].value();
      if (IsPlayerMatch(comp, player_id)) {
        local_entity_ = registry_.EntityFromIndex(i);
        MarkLocalPlayer(*local_entity_);
        return local_entity_;
      }
    }
  }

  for (std::size_t i = 0; i < net.size(); ++i) {
    if (!net[i].has_value()) {
      continue;
    }
    const auto& comp = net[i].value();
    if (archetypes.IsPlayer(comp.type_code) ||
        HasPlayerTag(registry_, registry_.EntityFromIndex(i))) {
      local_entity_ = registry_.EntityFromIndex(i);
      MarkLocalPlayer(*local_entity_);
      return local_entity_;
    }
  }

  return std::nullopt;
}

engine::math::Vector2f LocalPrediction::ComputeVelocity(
    const ActionState& input_state) {
  engine::math::Vector2f velocity{0.0f, 0.0f};
  if (input_state.move_left && !input_state.move_right) {
    velocity.x = -kMoveSpeed;
  } else if (input_state.move_right && !input_state.move_left) {
    velocity.x = kMoveSpeed;
  }

  if (input_state.move_up && !input_state.move_down) {
    velocity.y = -kMoveSpeed;
  } else if (input_state.move_down && !input_state.move_up) {
    velocity.y = kMoveSpeed;
  }
  return velocity;
}

void LocalPrediction::ClampPosition(engine::math::Vector2f& position) {
  position.x = std::clamp(position.x, 0.0f, kWorldWidth);
  position.y = std::clamp(position.y, 0.0f, kWorldHeight);
}

std::optional<engine::math::Vector2f>
LocalPrediction::CapturePredictedPosition() {
  const auto entity = ResolveLocalEntity();
  if (!entity.has_value()) {
    return std::nullopt;
  }
  const auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const std::size_t idx = static_cast<std::size_t>(*entity);
  if (idx >= positions.size() || !positions[idx].has_value()) {
    return std::nullopt;
  }
  return positions[idx]->position;
}

void LocalPrediction::OnSnapshotApplied(
    const std::optional<engine::math::Vector2f>& predicted_before) {
  const auto entity = ResolveLocalEntity();
  if (!entity.has_value()) {
    return;
  }

  auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const std::size_t idx = static_cast<std::size_t>(*entity);
  if (idx >= positions.size() || !positions[idx].has_value()) {
    return;
  }

  if (predicted_before.has_value()) {
    const auto authoritative = positions[idx]->position;
    reconciliation_offset_ = authoritative - *predicted_before;
    reconciliation_elapsed_ = 0.0f;
    positions[idx]->previous_position = *predicted_before;
    positions[idx]->position = *predicted_before;
    positions[idx]->render_position = positions[idx]->position;
  } else {
    reconciliation_offset_ = {0.0f, 0.0f};
    reconciliation_elapsed_ = 0.0f;
    positions[idx]->render_position = positions[idx]->position;
  }
}

void LocalPrediction::ApplyReconciliation(ecs::PositionComponent& position,
                                          float dt_seconds) {
  if (reconciliation_offset_ == engine::math::Vector2f{0.0f, 0.0f}) {
    return;
  }

  const float prev_progress =
      std::clamp(reconciliation_elapsed_ / kReconciliationDuration, 0.0f, 1.0f);
  reconciliation_elapsed_ =
      std::min(reconciliation_elapsed_ + dt_seconds, kReconciliationDuration);
  const float new_progress =
      std::clamp(reconciliation_elapsed_ / kReconciliationDuration, 0.0f, 1.0f);
  const float delta = new_progress - prev_progress;
  position.position += reconciliation_offset_ * delta;
  ClampPosition(position.position);
  position.render_position = position.position;

  if (reconciliation_elapsed_ >= kReconciliationDuration) {
    reconciliation_offset_ = {0.0f, 0.0f};
  }
}

void LocalPrediction::MarkLocalPlayer(engine::ecs::EntityId entity) {
  auto& locals = registry_.GetComponents<ecs::LocalPlayerTag>();
  const std::size_t idx = static_cast<std::size_t>(entity);
  if (idx >= locals.size() || !locals[idx].has_value()) {
    locals[entity] = ecs::LocalPlayerTag{};
  }
}

void LocalPrediction::Update(engine::time::TimeDelta dt,
                             const ActionState& input_state) {
  const auto entity = ResolveLocalEntity();
  if (!entity.has_value()) {
    return;
  }

  auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  auto& velocities = registry_.GetComponents<ecs::VelocityComponent>();
  const std::size_t idx = static_cast<std::size_t>(*entity);
  if (idx >= positions.size() || !positions[idx].has_value()) {
    return;
  }

  auto& pos = positions[idx].value();
  engine::math::Vector2f velocity = ComputeVelocity(input_state);
  if (idx < velocities.size()) {
    if (velocities[idx].has_value()) {
      velocities[idx]->velocity = velocity;
    } else {
      velocities[idx] = ecs::VelocityComponent(velocity);
    }
  }

  pos.previous_position = pos.position;
  pos.position += velocity * dt.as_seconds();
  ClampPosition(pos.position);
  pos.render_position = pos.position;
  ApplyReconciliation(pos, dt.as_seconds());
}

}  // namespace client
