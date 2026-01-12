#include "rift/systems/dodge_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void DodgeSystem::Update(engine::ecs::Registry& registry,
                         engine::time::TimeDelta dt) {
  auto& dodges = registry.GetComponents<components::DodgeComponent>();
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& healths = registry.GetComponents<components::HealthComponent>();
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();

  const float dt_ms = static_cast<float>(dt.as_milliseconds());
  const float dt_sec = dt.as_seconds();

  for (auto [idx, dodge, pos, health, combat] :
       engine::ecs::IndexedZipper(dodges, positions, healths, combat_states)) {
    if (dodge->cooldown_remaining_ms > 0.0f) {
      dodge->cooldown_remaining_ms -= dt_ms;
      if (dodge->cooldown_remaining_ms < 0.0f) {
        dodge->cooldown_remaining_ms = 0.0f;
      }
    }

    if (dodge->is_dodging) {
      dodge->current_time_ms += dt_ms;

      float move_dist = dodge->dodge_speed * dt_sec * static_cast<float>(dodge->direction);
      pos->position.x += move_dist;

      health->invulnerable = true;

      if (dodge->current_time_ms >= dodge->dodge_duration_ms) {
        dodge->is_dodging = false;
        dodge->current_time_ms = 0.0f;
        dodge->cooldown_remaining_ms = dodge->cooldown_ms;
        health->invulnerable = false;
        combat->state = components::CombatState::kIdle;
        combat->state_timer_ms = 0;
      }
    }
  }
}

}  // namespace rift::systems
