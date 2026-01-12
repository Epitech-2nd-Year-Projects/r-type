#include "rift/systems/block_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void BlockSystem::Update(engine::ecs::Registry& registry,
                         engine::time::TimeDelta) {
  auto& blocks = registry.GetComponents<components::BlockComponent>();
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();
  auto& attacks = registry.GetComponents<components::AttackComponent>();

  for (auto [idx, block, combat, attack] :
       engine::ecs::IndexedZipper(blocks, combat_states, attacks)) {
    if (combat->state == components::CombatState::kStunned) {
      block->is_blocking = false;
      continue;
    }

    if (attack->type != components::AttackType::kNone) {
      block->is_blocking = false;
    }
  }
}

}  // namespace rift::systems
