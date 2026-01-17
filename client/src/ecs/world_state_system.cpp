#include "ecs/world_state_system.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>

namespace client::ecs {

namespace {

constexpr float kQuantizationScale = 1.0f;

std::uint64_t TickDelta(std::uint32_t current_tick,
                        std::uint32_t anchor_tick) {
  // Unsigned wraparound keeps server tick rollover monotonic.
  return static_cast<std::uint32_t>(current_tick - anchor_tick);
}

std::uint64_t TickDurationMs(std::uint32_t tick_rate_hz) {
  return static_cast<std::uint64_t>(1000.0 /
                                    static_cast<double>(tick_rate_hz));
}

struct SnapshotTiming {
  std::uint64_t predicted_ms;
  std::uint64_t gap_ms;
};

SnapshotTiming PredictSnapshotTiming(std::uint32_t server_tick,
                                     std::uint32_t anchor_tick,
                                     std::uint64_t anchor_time_ms,
                                     std::uint64_t tick_ms,
                                     std::uint64_t observation_ms) {
  const std::uint64_t tick_delta = TickDelta(server_tick, anchor_tick);
  const std::uint64_t predicted_time = anchor_time_ms + tick_delta * tick_ms;
  const std::uint64_t gap_ms = observation_ms >= predicted_time
                                   ? observation_ms - predicted_time
                                   : predicted_time - observation_ms;
  return {predicted_time, gap_ms};
}

}  // namespace

WorldStateSystem::WorldStateSystem(engine::ecs::Registry &registry)
    : registry_(registry),
      archetypes_(ArchetypeRegistry::Get()),
      animation_factory_(archetypes_) {
  RegisterComponents();
}

void WorldStateSystem::RegisterComponents() {
  registry_.RegisterComponent<NetworkedEntityComponent>();
  registry_.RegisterComponent<PositionComponent>();
  registry_.RegisterComponent<VelocityComponent>();
  registry_.RegisterComponent<SpriteComponent>();
  registry_.RegisterComponent<RenderLayerComponent>();
  registry_.RegisterComponent<HealthComponent>();
  registry_.RegisterComponent<PlayerStateComponent>();
  registry_.RegisterComponent<PlayerTag>();
  registry_.RegisterComponent<EnemyTag>();
  registry_.RegisterComponent<MissileTag>();
  registry_.RegisterComponent<LocalPlayerTag>();
  registry_.RegisterComponent<AnimationComponent>();
  registry_.RegisterComponent<SnapshotInterpolationComponent>();
}

void WorldStateSystem::Reset() {
  for (const auto &[network_id, entity] : network_to_entity_) {
    (void)network_id;
    registry_.KillEntity(entity);
  }
  network_to_entity_.clear();
  last_snapshot_id_ = 0;
  local_player_entity_.reset();
  server_tick_rate_hz_.reset();
  server_time_anchor_ms_.reset();
  server_tick_anchor_.reset();
}

void WorldStateSystem::ApplySnapshot(
    const protocol::WorldSnapshotPayload &snapshot,
    std::uint64_t receipt_timestamp_ms,
    std::optional<std::uint32_t> local_player_id,
    std::optional<std::uint32_t> server_tick_rate_hz,
    std::optional<std::uint64_t> snapshot_time_ms) {
  if (snapshot.snapshot_id <= last_snapshot_id_) {
    return;
  }

  if (server_tick_rate_hz.has_value()) {
    server_tick_rate_hz_ = server_tick_rate_hz;
  }

  const std::uint64_t observation_ms =
      snapshot_time_ms.value_or(receipt_timestamp_ms);
  const std::uint32_t tick_rate =
      server_tick_rate_hz.value_or(server_tick_rate_hz_.value_or(0));
  if (tick_rate > 0) {
    const std::uint64_t tick_ms = TickDurationMs(tick_rate);
    if (!server_time_anchor_ms_.has_value() ||
        !server_tick_anchor_.has_value()) {
      server_time_anchor_ms_ = observation_ms;
      server_tick_anchor_ = snapshot.server_tick;
    } else {
      const auto timing =
          PredictSnapshotTiming(snapshot.server_tick,
                                static_cast<std::uint32_t>(
                                    server_tick_anchor_.value()),
                                server_time_anchor_ms_.value(), tick_ms,
                                observation_ms);
      if (timing.gap_ms > (tick_ms * 2u)) {
        server_time_anchor_ms_ = observation_ms;
        server_tick_anchor_ = snapshot.server_tick;
      }
    }
  }

  const auto resolved_time_ms =
      ResolveSnapshotTimeMs(snapshot, server_tick_rate_hz,
                            std::optional<std::uint64_t>(observation_ms));

  const bool full_snapshot =
      snapshot.base_snapshot_id == protocol::kNoBaseSnapshotId;
  std::unordered_set<std::uint32_t> seen;
  seen.reserve(snapshot.deltas.size());

  for (const auto &delta : snapshot.deltas) {
    switch (delta.op) {
      case protocol::EntityDeltaOp::kCreate:
        ApplyCreate(delta, snapshot.snapshot_id, receipt_timestamp_ms,
                    resolved_time_ms);
        seen.insert(delta.entity_id);
        break;
      case protocol::EntityDeltaOp::kUpdate:
        ApplyUpdate(delta, snapshot.snapshot_id, receipt_timestamp_ms,
                    resolved_time_ms);
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

  if (local_player_id.has_value()) {
    const auto &player_states = registry_.GetComponents<PlayerStateComponent>();
    const auto &net = registry_.GetComponents<NetworkedEntityComponent>();
    bool tagged = false;
    for (std::size_t i = 0; i < player_states.size(); ++i) {
      if (!player_states[i].has_value()) {
        continue;
      }
      if (player_states[i]->player_id != local_player_id.value()) {
        continue;
      }
      if (i >= net.size() || !net[i].has_value()) {
        continue;
      }
      const auto entity = registry_.EntityFromIndex(i);
      UpdateLocalPlayerTag(entity, local_player_id);
      tagged = true;
      break;
    }
    if (!tagged && local_player_entity_.has_value()) {
      UpdateLocalPlayerTag(local_player_entity_.value(), std::nullopt);
    }
  } else if (local_player_entity_.has_value()) {
    UpdateLocalPlayerTag(local_player_entity_.value(), std::nullopt);
  }

  last_snapshot_id_ = snapshot.snapshot_id;
}

void WorldStateSystem::ApplyCreate(
    const protocol::EntityDelta &delta, std::uint32_t snapshot_id,
    std::uint64_t receipt_timestamp_ms,
    std::optional<std::uint64_t> snapshot_time_ms) {
  const auto entity =
      ResolveOrCreateEntity(delta.entity_id, snapshot_id, delta.state.type);

  auto &net = registry_.GetComponents<NetworkedEntityComponent>();
  auto &net_comp = net[entity].value();
  net_comp.type_code = delta.state.type;
  net_comp.last_snapshot = snapshot_id;
  net_comp.flags = delta.state.flags;

  UpdateArchetypeTags(entity, delta.state.type);

  const auto position = ToVector(delta.state.x, delta.state.y);
  auto &positions = registry_.GetComponents<PositionComponent>();
  positions[entity] = PositionComponent(position, position);
  positions[entity]->render_position = position;

  const auto velocity = ToVector(delta.state.vx, delta.state.vy);
  auto &velocities = registry_.GetComponents<VelocityComponent>();
  velocities[entity] = VelocityComponent(velocity);

  auto &health = registry_.GetComponents<HealthComponent>();
  const auto hp = delta.state.hp;
  auto &target_hp = health[entity];
  if (!target_hp.has_value()) {
    target_hp = HealthComponent(static_cast<std::uint32_t>(hp),
                                static_cast<std::uint32_t>(hp));
  } else {
    target_hp->max = std::max(target_hp->max, static_cast<std::uint32_t>(hp));
    target_hp->current = static_cast<std::uint32_t>(hp);
  }

  animation_factory_.EnsureAnimation(registry_, entity, delta.state.type);

  auto &player_states = registry_.GetComponents<PlayerStateComponent>();
  if (archetypes_.IsPlayer(delta.state.type)) {
    player_states[entity] = PlayerStateComponent(
        delta.state.player_id, delta.state.score, delta.state.lives);
  }

  const std::uint64_t timing_ms =
      snapshot_time_ms.value_or(receipt_timestamp_ms);
  auto &snapshots = registry_.GetComponents<SnapshotInterpolationComponent>();
  snapshots[entity] = SnapshotInterpolationComponent(timing_ms, timing_ms);
}

void WorldStateSystem::ApplyUpdate(
    const protocol::EntityDelta &delta, std::uint32_t snapshot_id,
    std::uint64_t receipt_timestamp_ms,
    std::optional<std::uint64_t> snapshot_time_ms) {
  const auto it = network_to_entity_.find(delta.entity_id);
  const bool created = it == network_to_entity_.end();
  const auto entity = created
                          ? ResolveOrCreateEntity(delta.entity_id, snapshot_id,
                                                  delta.state.type)
                          : it->second;
  auto &net = registry_.GetComponents<NetworkedEntityComponent>();
  auto &net_comp = net[entity];
  auto &player_states = registry_.GetComponents<PlayerStateComponent>();
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
  if (delta.field_mask & protocol::EntityFieldMask::kFieldFlags) {
    net_comp->flags = delta.state.flags;
  }
  net_comp->last_snapshot = snapshot_id;

  const bool needs_animation =
      created || (delta.field_mask & protocol::EntityFieldMask::kFieldType);
  if (needs_animation) {
    UpdateArchetypeTags(entity, net_comp->type_code);
  }

  const bool is_player_type = archetypes_.IsPlayer(
      (delta.field_mask & protocol::EntityFieldMask::kFieldType)
          ? delta.state.type
          : net_comp->type_code);

  if (delta.field_mask & protocol::EntityFieldMask::kFieldX ||
      delta.field_mask & protocol::EntityFieldMask::kFieldY) {
    auto &positions = registry_.GetComponents<PositionComponent>();
    auto &pos = positions[entity];
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
    auto &velocities = registry_.GetComponents<VelocityComponent>();
    auto &vel = velocities[entity];
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
    auto &health = registry_.GetComponents<HealthComponent>();
    auto &hp = health[entity];
    if (!hp.has_value()) {
      hp = HealthComponent(static_cast<std::uint32_t>(delta.state.hp),
                           static_cast<std::uint32_t>(delta.state.hp));
    } else {
      hp->current = static_cast<std::uint32_t>(delta.state.hp);
      hp->max = std::max(hp->max, static_cast<std::uint32_t>(delta.state.hp));
    }
  }

  if (needs_animation) {
    animation_factory_.EnsureAnimation(registry_, entity, net_comp->type_code);
  }

  if (is_player_type) {
    auto &player_state = player_states[entity];
    if (!player_state.has_value()) {
      player_state = PlayerStateComponent(delta.state.player_id,
                                          delta.state.score, delta.state.lives);
    }
    if (delta.field_mask & protocol::EntityFieldMask::kFieldScore) {
      player_state->score = delta.state.score;
    }
    if (delta.field_mask & protocol::EntityFieldMask::kFieldLives) {
      player_state->lives = delta.state.lives;
    }
    if (delta.field_mask & protocol::EntityFieldMask::kFieldPlayerId) {
      player_state->player_id = delta.state.player_id;
    }
  } else if (entity < player_states.size() &&
             player_states[entity].has_value()) {
    player_states[entity].reset();
  }

  const std::uint64_t timing_ms =
      snapshot_time_ms.value_or(receipt_timestamp_ms);
  auto &snapshots = registry_.GetComponents<SnapshotInterpolationComponent>();
  auto &snapshot = snapshots[entity];
  if (!snapshot.has_value()) {
    snapshot = SnapshotInterpolationComponent(timing_ms, timing_ms);
  } else {
    if (timing_ms >= snapshot->last_snapshot_ms) {
      snapshot->previous_snapshot_ms = snapshot->last_snapshot_ms;
      snapshot->last_snapshot_ms = timing_ms;
    }
  }
}

void WorldStateSystem::ApplyDelete(const protocol::EntityDelta &delta) {
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

void WorldStateSystem::UpdateArchetypeTags(engine::ecs::EntityId entity,
                                           std::uint16_t type_code) {
  const auto kind = archetypes_.KindOf(type_code);

  auto &players = registry_.GetComponents<PlayerTag>();
  auto &enemies = registry_.GetComponents<EnemyTag>();
  auto &missiles = registry_.GetComponents<MissileTag>();

  const bool is_player = kind == ArchetypeKind::kPlayer;
  const bool is_enemy = kind == ArchetypeKind::kEnemy;
  const bool is_missile = kind == ArchetypeKind::kMissile;

  if (is_player) {
    players[entity] = PlayerTag{};
  } else if (players[entity].has_value()) {
    players[entity].reset();
  }
  if (is_enemy) {
    enemies[entity] = EnemyTag{};
  } else if (enemies[entity].has_value()) {
    enemies[entity].reset();
  }
  if (is_missile) {
    missiles[entity] = MissileTag{};
  } else if (missiles[entity].has_value()) {
    missiles[entity].reset();
  }
}

engine::math::Vector2f WorldStateSystem::ToVector(std::int16_t x,
                                                  std::int16_t y) {
  return {static_cast<float>(x) * kQuantizationScale,
          static_cast<float>(y) * kQuantizationScale};
}

void WorldStateSystem::UpdateLocalPlayerTag(
    engine::ecs::EntityId entity,
    std::optional<std::uint32_t> local_player_id) {
  auto &locals = registry_.GetComponents<LocalPlayerTag>();
  if (!local_player_id.has_value()) {
    if (locals[entity].has_value()) {
      locals[entity].reset();
    }
    if (local_player_entity_.has_value() &&
        local_player_entity_.value() == entity) {
      local_player_entity_.reset();
    }
    return;
  }
  locals[entity] = LocalPlayerTag{};
  local_player_entity_ = entity;
}

std::optional<std::uint64_t> WorldStateSystem::ResolveSnapshotTimeMs(
    const protocol::WorldSnapshotPayload &snapshot,
    std::optional<std::uint32_t> server_tick_rate_hz,
    std::optional<std::uint64_t> snapshot_time_ms) const {
  if (!snapshot_time_ms.has_value()) {
    return std::nullopt;
  }
  const std::uint64_t server_time_value = snapshot_time_ms.value();
  const std::uint32_t tick_rate =
      server_tick_rate_hz.value_or(server_tick_rate_hz_.value_or(0));
  if (tick_rate == 0) {
    return std::nullopt;
  }
  const std::uint64_t tick_ms = TickDurationMs(tick_rate);
  if (!server_time_anchor_ms_.has_value() || !server_tick_anchor_.has_value()) {
    return server_time_value;
  }
  const auto timing = PredictSnapshotTiming(
      snapshot.server_tick,
      static_cast<std::uint32_t>(server_tick_anchor_.value()),
      server_time_anchor_ms_.value(), tick_ms, server_time_value);
  if (timing.gap_ms > (tick_ms * 2u)) {
    return server_time_value;
  }
  return timing.predicted_ms;
}

}  // namespace client::ecs
