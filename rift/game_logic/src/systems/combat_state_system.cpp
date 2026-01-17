#include "rift/systems/combat_state_system.h"

#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void CombatStateSystem::Update(engine::ecs::Registry& registry,
                               engine::time::TimeDelta dt) {
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();
  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto& attacks = registry.GetComponents<components::AttackComponent>();
  auto& blocks = registry.GetComponents<components::BlockComponent>();
  auto& dodges = registry.GetComponents<components::DodgeComponent>();

  const auto dt_ms = static_cast<std::uint32_t>(dt.as_milliseconds());

  for (auto [idx, combat, vel] :
       engine::ecs::IndexedZipper(combat_states, velocities)) {
    combat->state_timer_ms += dt_ms;

    switch (combat->state) {
      case components::CombatState::kStunned:
        if (combat->state_timer_ms >= combat->stun_duration_ms) {
          combat->state = components::CombatState::kIdle;
          combat->state_timer_ms = 0;
        }
        break;

      case components::CombatState::kIdle:
        if (vel->velocity.x != 0.0f) {
          combat->state = components::CombatState::kWalking;
          combat->state_timer_ms = 0;
        }
        break;

      case components::CombatState::kWalking:
        if (vel->velocity.x == 0.0f) {
          combat->state = components::CombatState::kIdle;
          combat->state_timer_ms = 0;
        }
        break;

      default:
        break;
    }
  }

  for (auto [idx, combat, attack] :
       engine::ecs::IndexedZipper(combat_states, attacks)) {
    if (attack->type != components::AttackType::kNone) {
      if (combat->state != components::CombatState::kAttacking) {
        combat->state = components::CombatState::kAttacking;
        combat->state_timer_ms = 0;
      }
    } else if (combat->state == components::CombatState::kAttacking) {
      combat->state = components::CombatState::kIdle;
      combat->state_timer_ms = 0;
    }
  }

  for (auto [idx, combat, block] :
       engine::ecs::IndexedZipper(combat_states, blocks)) {
    if (block->is_blocking && combat->state != components::CombatState::kStunned) {
      if (combat->state != components::CombatState::kBlocking) {
        combat->state = components::CombatState::kBlocking;
        combat->state_timer_ms = 0;
      }
    } else if (combat->state == components::CombatState::kBlocking) {
      combat->state = components::CombatState::kIdle;
      combat->state_timer_ms = 0;
    }
  }

  for (auto [idx, combat, dodge] :
       engine::ecs::IndexedZipper(combat_states, dodges)) {
    if (dodge->is_dodging) {
      if (combat->state != components::CombatState::kDodging) {
        combat->state = components::CombatState::kDodging;
        combat->state_timer_ms = 0;
      }
    } else if (combat->state == components::CombatState::kDodging) {
      combat->state = components::CombatState::kIdle;
      combat->state_timer_ms = 0;
    }
  }
}

}  // namespace rift::systems
