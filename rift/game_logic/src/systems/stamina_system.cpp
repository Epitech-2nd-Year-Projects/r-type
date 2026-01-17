#include "rift/systems/stamina_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void StaminaSystem::Update(engine::ecs::Registry& registry,
                           engine::time::TimeDelta dt) {
  auto& staminas = registry.GetComponents<components::StaminaComponent>();
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();
  auto& blocks = registry.GetComponents<components::BlockComponent>();

  const float dt_sec = dt.as_seconds();

  for (auto [idx, stamina, combat] :
       engine::ecs::IndexedZipper(staminas, combat_states)) {
    bool can_regen = combat->state == components::CombatState::kIdle ||
                     combat->state == components::CombatState::kWalking;
    stamina->regenerating = can_regen;

    if (can_regen) {
      stamina->Regenerate(dt_sec);
    }
  }

  for (auto [idx, stamina, block, combat] :
       engine::ecs::IndexedZipper(staminas, blocks, combat_states)) {
    if (block->is_blocking) {
      float drain = block->stamina_drain_rate * dt_sec;
      if (!stamina->Consume(drain)) {
        block->is_blocking = false;
        combat->state = components::CombatState::kStunned;
        combat->state_timer_ms = 0;
        combat->stun_duration_ms =
            static_cast<std::uint32_t>(components::BlockComponent::kGuardBreakRecoveryMs);
      }
    }
  }
}

}  // namespace rift::systems
