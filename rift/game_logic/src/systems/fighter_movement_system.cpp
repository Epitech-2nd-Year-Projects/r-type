#include "rift/systems/fighter_movement_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void FighterMovementSystem::Update(engine::ecs::Registry& registry,
                                   engine::time::TimeDelta dt) {
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto& fighters = registry.GetComponents<components::FighterComponent>();
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();

  const float dt_sec = dt.as_seconds();

  for (auto [idx, pos, vel, fighter, combat] :
       engine::ecs::IndexedZipper(positions, velocities, fighters, combat_states)) {
    if (pos->position.y < kGroundY) {
      vel->velocity.y += kGravity * dt_sec;
    }

    pos->position.y += vel->velocity.y * dt_sec;

    if (pos->position.y >= kGroundY) {
      pos->position.y = kGroundY;
      vel->velocity.y = 0.0f;
    }

    if (combat->state == components::CombatState::kStunned ||
        combat->state == components::CombatState::kDodging) {
      continue;
    }

    pos->position.x += vel->velocity.x * dt_sec;
    pos->position.x =
        std::clamp(pos->position.x, 0.0f, kArenaWidth - kFighterWidth);

    if (vel->velocity.x != 0.0f) {
      fighter->facing_right = vel->velocity.x > 0.0f;
    }
  }

  std::vector<std::pair<std::size_t, engine::ecs::PositionComponent*>> fighter_positions;
  for (auto [idx, pos, fighter] :
       engine::ecs::IndexedZipper(positions, fighters)) {
    fighter_positions.emplace_back(fighter->slot, &(*pos));
  }

  if (fighter_positions.size() == 2) {
    auto* p1 = fighter_positions[0].first == 0
        ? fighter_positions[0].second
        : fighter_positions[1].second;
    auto* p2 = fighter_positions[0].first == 1
        ? fighter_positions[0].second
        : fighter_positions[1].second;

    float dist = p2->position.x - p1->position.x;
    if (dist < kMinDistance) {
      float push = (kMinDistance - dist) / 2.0f;
      p1->position.x -= push;
      p2->position.x += push;

      p1->position.x =
          std::clamp(p1->position.x, 0.0f, kArenaWidth - kFighterWidth);
      p2->position.x =
          std::clamp(p2->position.x, 0.0f, kArenaWidth - kFighterWidth);
    }
  }
}

}  // namespace rift::systems
