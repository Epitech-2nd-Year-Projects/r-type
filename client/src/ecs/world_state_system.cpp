#include "ecs/world_state_system.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace client::ecs {

namespace {

constexpr float kQuantizationScale = 1.0f;

}  // namespace

WorldStateSystem::WorldStateSystem(engine::ecs::Registry& registry)
    : registry_(registry) {
  RegisterComponents();
}

void WorldStateSystem::RegisterComponents() {
  registry_.RegisterComponent<NetworkedEntityComponent>();
  registry_.RegisterComponent<PositionComponent>();
  registry_.RegisterComponent<VelocityComponent>();
  registry_.RegisterComponent<SpriteComponent>();
  registry_.RegisterComponent<RenderLayerComponent>();
  registry_.RegisterComponent<HealthComponent>();
  registry_.RegisterComponent<PlayerTag>();
  registry_.RegisterComponent<EnemyTag>();
  registry_.RegisterComponent<MissileTag>();
  registry_.RegisterComponent<LocalPlayerTag>();
  registry_.RegisterComponent<SnapshotInterpolationComponent>();
}

void WorldStateSystem::Reset() {
  for (const auto& [network_id, entity] : network_to_entity_) {
    (void)network_id;
    registry_.KillEntity(entity);
  }
  network_to_entity_.clear();
  last_snapshot_id_ = 0;
}

void WorldStateSystem::ApplySnapshot(
    const protocol::WorldSnapshotPayload& snapshot,
    std::uint64_t receipt_timestamp_ms) {
  if (snapshot.snapshot_id <= last_snapshot_id_) {
    return;
  }

  const bool full_snapshot =
      snapshot.base_snapshot_id == protocol::kNoBaseSnapshotId;
  std::unordered_set<std::uint32_t> seen;
  seen.reserve(snapshot.deltas.size());

  for (const auto& delta : snapshot.deltas) {
    switch (delta.op) {
      case protocol::EntityDeltaOp::kCreate:
        ApplyCreate(delta, snapshot.snapshot_id, receipt_timestamp_ms);
        seen.insert(delta.entity_id);
        break;
      case protocol::EntityDeltaOp::kUpdate:
        ApplyUpdate(delta, snapshot.snapshot_id, receipt_timestamp_ms);
        seen.insert(delta.entity_id);
        break;
      case protocol::EntityDeltaOp::kDelete:
        ApplyDelete(delta);
        break;
    }
  }

  if (full_snapshot) {
    for (auto it = network_to_entity_.begin();
         it != network_to_entity_.end();) {
      if (seen.find(it->first) == seen.end()) {
        registry_.KillEntity(it->second);
        it = network_to_entity_.erase(it);
      } else {
        ++it;
      }
    }
  }

  last_snapshot_id_ = snapshot.snapshot_id;
}

void WorldStateSystem::ApplyCreate(const protocol::EntityDelta& delta,
                                   std::uint32_t snapshot_id,
                                   std::uint64_t receipt_timestamp_ms) {
  const auto entity =
      ResolveOrCreateEntity(delta.entity_id, snapshot_id, delta.state.type);

  auto& net = registry_.GetComponents<NetworkedEntityComponent>();
  auto& net_comp = net[entity].value();
  net_comp.type_code = delta.state.type;
  net_comp.last_snapshot = snapshot_id;

  const auto position = ToVector(delta.state.x, delta.state.y);
  auto& positions = registry_.GetComponents<PositionComponent>();
  positions[entity] = PositionComponent(position, position);
  positions[entity]->render_position = position;

  const auto velocity = ToVector(delta.state.vx, delta.state.vy);
  auto& velocities = registry_.GetComponents<VelocityComponent>();
  velocities[entity] = VelocityComponent(velocity);

  auto& health = registry_.GetComponents<HealthComponent>();
  const auto hp = delta.state.hp;
  health[entity] = HealthComponent(hp, hp);

  auto& snapshots = registry_.GetComponents<SnapshotInterpolationComponent>();
  snapshots[entity] = SnapshotInterpolationComponent(receipt_timestamp_ms,
                                                     receipt_timestamp_ms);
}

void WorldStateSystem::ApplyUpdate(const protocol::EntityDelta& delta,
                                   std::uint32_t snapshot_id,
                                   std::uint64_t receipt_timestamp_ms) {
  const auto it = network_to_entity_.find(delta.entity_id);
  const bool created = it == network_to_entity_.end();
  const auto entity = created
                          ? ResolveOrCreateEntity(delta.entity_id, snapshot_id,
                                                  delta.state.type)
                          : it->second;
  auto& net = registry_.GetComponents<NetworkedEntityComponent>();
  auto& net_comp = net[entity];
  const std::uint32_t last_applied =
      net_comp.has_value() ? net_comp->last_snapshot : 0u;
  if (!created && last_applied >= snapshot_id) {
    return;
  }
  if (!net_comp.has_value()) {
    net_comp = NetworkedEntityComponent{delta.entity_id, delta.state.type,
                                        snapshot_id};
  }

  if (delta.field_mask & protocol::EntityFieldMask::kFieldType) {
    net_comp->type_code = delta.state.type;
  }
  net_comp->last_snapshot = snapshot_id;

  if (delta.field_mask & protocol::EntityFieldMask::kFieldX ||
      delta.field_mask & protocol::EntityFieldMask::kFieldY) {
    auto& positions = registry_.GetComponents<PositionComponent>();
    auto& pos = positions[entity];
    const float target_x =
        delta.field_mask & protocol::EntityFieldMask::kFieldX
            ? static_cast<float>(delta.state.x) * kQuantizationScale
            : (pos ? pos->position.x : 0.0f);
    const float target_y =
        delta.field_mask & protocol::EntityFieldMask::kFieldY
            ? static_cast<float>(delta.state.y) * kQuantizationScale
            : (pos ? pos->position.y : 0.0f);
    const engine::math::Vector2f target{target_x, target_y};
    if (!pos.has_value()) {
      pos = PositionComponent(target, target);
      pos->render_position = target;
    } else {
      pos->previous_position = pos->position;
      pos->position = target;
      pos->render_position = pos->previous_position;
    }
  }

  if (delta.field_mask & protocol::EntityFieldMask::kFieldVx ||
      delta.field_mask & protocol::EntityFieldMask::kFieldVy) {
    auto& velocities = registry_.GetComponents<VelocityComponent>();
    auto& vel = velocities[entity];
    const float target_x =
        delta.field_mask & protocol::EntityFieldMask::kFieldVx
            ? static_cast<float>(delta.state.vx) * kQuantizationScale
            : (vel ? vel->velocity.x : 0.0f);
    const float target_y =
        delta.field_mask & protocol::EntityFieldMask::kFieldVy
            ? static_cast<float>(delta.state.vy) * kQuantizationScale
            : (vel ? vel->velocity.y : 0.0f);
    const engine::math::Vector2f target{target_x, target_y};
    if (!vel.has_value()) {
      vel = VelocityComponent(target);
    } else {
      vel->velocity = target;
    }
  }

  if (delta.field_mask & protocol::EntityFieldMask::kFieldHp) {
    auto& health = registry_.GetComponents<HealthComponent>();
    auto& hp = health[entity];
    if (!hp.has_value()) {
      hp = HealthComponent(delta.state.hp, delta.state.hp);
    } else {
      hp->current = delta.state.hp;
      hp->max = std::max(hp->max, delta.state.hp);
    }
  }

  auto& snapshots = registry_.GetComponents<SnapshotInterpolationComponent>();
  auto& snapshot = snapshots[entity];
  if (!snapshot.has_value()) {
    snapshot = SnapshotInterpolationComponent(receipt_timestamp_ms,
                                              receipt_timestamp_ms);
  } else {
    snapshot->previous_snapshot_ms = snapshot->last_snapshot_ms;
    snapshot->last_snapshot_ms = receipt_timestamp_ms;
  }
}

void WorldStateSystem::ApplyDelete(const protocol::EntityDelta& delta) {
  const auto it = network_to_entity_.find(delta.entity_id);
  if (it == network_to_entity_.end()) {
    return;
  }
  registry_.KillEntity(it->second);
  network_to_entity_.erase(it);
}

engine::ecs::EntityId WorldStateSystem::ResolveOrCreateEntity(
    std::uint32_t network_id, std::uint32_t snapshot_id,
    std::uint16_t type_code) {
  const auto it = network_to_entity_.find(network_id);
  if (it != network_to_entity_.end()) {
    return it->second;
  }

  const auto entity = registry_.SpawnEntity();
  registry_.EmplaceComponent<NetworkedEntityComponent>(entity, network_id,
                                                       type_code, snapshot_id);
  network_to_entity_.emplace(network_id, entity);
  return entity;
}

engine::math::Vector2f WorldStateSystem::ToVector(std::int16_t x,
                                                  std::int16_t y) {
  return {static_cast<float>(x) * kQuantizationScale,
          static_cast<float>(y) * kQuantizationScale};
}

}  // namespace client::ecs
