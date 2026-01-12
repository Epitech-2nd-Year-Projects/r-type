#include "rift/systems/damage_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"
#include "rift/game_instance.h"

namespace rift::systems {

DamageSystem::DamageSystem(GameInstance& instance) : instance_(instance) {}

void DamageSystem::Update(engine::ecs::Registry& registry,
                          engine::time::TimeDelta) {
  auto& healths = registry.GetComponents<components::HealthComponent>();
  auto& fighters = registry.GetComponents<components::FighterComponent>();

  for (auto [idx, health, fighter] :
       engine::ecs::IndexedZipper(healths, fighters)) {
    if (!health->IsAlive()) {
      instance_.OnPlayerDeath(fighter->player_id, 0);
    }
  }
}

}  // namespace rift::systems
