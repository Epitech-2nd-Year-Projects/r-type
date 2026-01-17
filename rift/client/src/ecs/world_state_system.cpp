#include "ecs/world_state_system.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "ecs/components.h"
#include "engine/render/color.h"
#include "rift/arena_constants.h"

namespace rift::client::ecs {

namespace {

/// @brief Server flags for fighter entities.
constexpr std::uint8_t kFlagInvulnerable = 1u;
constexpr std::uint8_t kFlagReady = 2u;
constexpr std::uint8_t kFlagFacingLeft = 4u;

/// @brief Default fighter rendering dimensions.
constexpr float kFighterWidth = rift::ArenaConstants::kFighterWidth;
constexpr float kFighterHeight = rift::ArenaConstants::kFighterHeight;

}  // namespace

WorldStateSystem::WorldStateSystem(engine::ecs::Registry& registry)
    : registry_(registry) {}

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
    std::optional<std::uint32_t> local_player_id,
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
        ApplyCreate(delta, local_player_id, snapshot.snapshot_id,
                    receipt_timestamp_ms);
        seen.insert(delta.entity_id);
        break;
      case protocol::EntityDeltaOp::kUpdate:
        ApplyUpdate(delta, local_player_id, snapshot.snapshot_id,
                    receipt_timestamp_ms);
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

void WorldStateSystem::ApplyCreate(
    const protocol::EntityDelta& delta,
    std::optional<std::uint32_t> local_player_id, std::uint32_t snapshot_id,
    std::uint64_t receipt_timestamp_ms) {
  const auto entity =
      ResolveOrCreateEntity(delta.entity_id, snapshot_id, delta.state.type);

  auto& net = registry_.GetComponents<NetworkedEntityComponent>();
  auto& net_comp = net[entity].value();
  net_comp.type_code = delta.state.type;
  net_comp.last_snapshot = snapshot_id;
  net_comp.flags = delta.state.flags;

  const engine::math::Vector2f position{static_cast<float>(delta.state.x),
                                        static_cast<float>(delta.state.y)};
  auto& positions = registry_.GetComponents<PositionComponent>();
  positions[entity] = PositionComponent(position.x, position.y);

  const engine::math::Vector2f velocity{static_cast<float>(delta.state.vx),
                                        static_cast<float>(delta.state.vy)};
  auto& velocities = registry_.GetComponents<VelocityComponent>();
  velocities[entity] = VelocityComponent(velocity.x, velocity.y);

  if (static_cast<EntityType>(delta.state.type) == EntityType::kFighter) {
    SetupFighterComponents(entity, delta.state, local_player_id);
  }

  auto& snapshots = registry_.GetComponents<SnapshotInterpolationComponent>();
  snapshots[entity] =
      SnapshotInterpolationComponent(receipt_timestamp_ms, receipt_timestamp_ms);
}

void WorldStateSystem::ApplyUpdate(
    const protocol::EntityDelta& delta,
    std::optional<std::uint32_t> local_player_id, std::uint32_t snapshot_id,
    std::uint64_t receipt_timestamp_ms) {
  const auto it = network_to_entity_.find(delta.entity_id);
  const bool created = it == network_to_entity_.end();
  const auto entity =
      created ? ResolveOrCreateEntity(delta.entity_id, snapshot_id,
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
    net_comp =
        NetworkedEntityComponent{delta.entity_id, delta.state.type, snapshot_id};
  }

  if (delta.field_mask & protocol::EntityFieldMask::kFieldType) {
    net_comp->type_code = delta.state.type;
  }
  if (delta.field_mask & protocol::EntityFieldMask::kFieldFlags) {
    net_comp->flags = delta.state.flags;
  }
  net_comp->last_snapshot = snapshot_id;

  if (delta.field_mask & protocol::EntityFieldMask::kFieldX ||
      delta.field_mask & protocol::EntityFieldMask::kFieldY) {
    auto& positions = registry_.GetComponents<PositionComponent>();
    auto& pos = positions[entity];
    const float target_x = delta.field_mask & protocol::EntityFieldMask::kFieldX
                               ? static_cast<float>(delta.state.x)
                               : (pos ? pos->position.x : 0.0f);
    const float target_y = delta.field_mask & protocol::EntityFieldMask::kFieldY
                               ? static_cast<float>(delta.state.y)
                               : (pos ? pos->position.y : 0.0f);
    const engine::math::Vector2f target{target_x, target_y};
    if (!pos.has_value()) {
      pos = PositionComponent(target.x, target.y);
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
            ? static_cast<float>(delta.state.vx)
            : (vel ? vel->velocity.x : 0.0f);
    const float target_y =
        delta.field_mask & protocol::EntityFieldMask::kFieldVy
            ? static_cast<float>(delta.state.vy)
            : (vel ? vel->velocity.y : 0.0f);
    if (!vel.has_value()) {
      vel = VelocityComponent(target_x, target_y);
    } else {
      vel->velocity = {target_x, target_y};
    }
  }

  const bool is_fighter =
      static_cast<EntityType>(net_comp->type_code) == EntityType::kFighter;
  if (is_fighter) {
    auto& fighters = registry_.GetComponents<FighterStateComponent>();
    auto& fighter = fighters[entity];
    if (!fighter.has_value()) {
      SetupFighterComponents(entity, delta.state, local_player_id);
    } else {
      if (delta.field_mask & protocol::EntityFieldMask::kFieldFlags) {
        fighter->facing_right = (delta.state.flags & kFlagFacingLeft) == 0;
      }
      if (delta.field_mask & protocol::EntityFieldMask::kFieldScore) {
        fighter->rounds_won = static_cast<std::uint8_t>(delta.state.score);
      }
    }

    if (delta.field_mask & protocol::EntityFieldMask::kFieldHp) {
      auto& health_bars = registry_.GetComponents<HealthBarComponent>();
      auto& hp = health_bars[entity];
      if (!hp.has_value()) {
        hp = HealthBarComponent(delta.state.hp, 100);
      } else {
        hp->current = delta.state.hp;
      }
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

void WorldStateSystem::SetupFighterComponents(
    engine::ecs::EntityId entity, const protocol::EntityNetState& state,
    std::optional<std::uint32_t> local_player_id) {
  const bool facing_right = (state.flags & kFlagFacingLeft) == 0;

  auto& fighters = registry_.GetComponents<FighterStateComponent>();
  auto& tags = registry_.GetComponents<FighterTag>();
  auto& local_tags = registry_.GetComponents<LocalFighterTag>();
  auto& renders = registry_.GetComponents<FighterRenderComponent>();
  auto& health_bars = registry_.GetComponents<HealthBarComponent>();
  auto& stamina_bars = registry_.GetComponents<StaminaBarComponent>();

  std::uint8_t slot = 0;
  if (fighters[entity].has_value()) {
    slot = fighters[entity]->slot;
  } else {
    const float arena_center = rift::ArenaConstants::kArenaWidth / 2.0f;
    const float spawn_x = static_cast<float>(state.x);
    slot = (spawn_x < arena_center) ? 0 : 1;
  }

  fighters[entity] = FighterStateComponent(state.player_id, slot);
  fighters[entity]->rounds_won = static_cast<std::uint8_t>(state.score);
  fighters[entity]->facing_right = facing_right;

  tags[entity] = FighterTag{};

  if (local_player_id.has_value() &&
      state.player_id == local_player_id.value()) {
    local_tags[entity] = LocalFighterTag{};
  }

  const auto color =
      slot == 0 ? engine::render::Color::FromBytes(50, 100, 255)
                : engine::render::Color::FromBytes(255, 80, 80);
  renders[entity] = FighterRenderComponent(kFighterWidth, kFighterHeight, color);

  const std::uint32_t hp = state.hp > 0 ? state.hp : 100;
  health_bars[entity] = HealthBarComponent(hp, 100);
  stamina_bars[entity] = StaminaBarComponent(100.0f, 100.0f);
}

}  // namespace rift::client::ecs
