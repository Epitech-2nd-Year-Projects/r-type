#include "game_logic/systems/health_system.h"

#include <vector>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/zipper.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/util/logging.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/game_config.h"
#include "game_logic/game_instance.h"

namespace game_logic::systems {

void HealthSystem::Update(engine::ecs::Registry &registry,
                          engine::time::TimeDelta) {
  auto &healths = registry.GetComponents<components::HealthComponent>();

  sol::state& lua = script_engine_.LuaState();
  sol::table game_events = lua["GameEvents"];
  sol::function handle_death;
  
  if (game_events.valid()) {
    handle_death = game_events["HandleDeath"];
  }

  // Iterate over health components
  // We cannot modify the registry (kill entities) while iterating if using zipped view?
  // Actually IndexedZipper is fine, but Killing invalidates iterators?
  // Registry::KillEntity usually defers or marks for deletion?
  // Wait, KillEntity usually verifies validity.
  // Standard practice: Collect dead entities, then process.
  
  // However, we need to call Lua ONLY ONCE per death.
  // If we don't kill the entity (e.g. respawn), hp > 0.
  // If we kill it, it's gone.
  
  // Logic: Find dead entities. Call HandleDeath.
  // HandleDeath (Lua) will either Heal (Respawn) or Kill (Registry:kill).
  
  // We use a list to avoid iterator invalidation during the loop if HandleDeath kills immediately.
  std::vector<engine::ecs::EntityId> dead_entities;

  for (auto [entity_idx, hp] : engine::ecs::IndexedZipper(healths)) {
    if (!hp->is_alive()) {
        dead_entities.push_back(registry.EntityFromIndex(entity_idx));
    }
  }

  if (handle_death.valid()) {
      for (auto entity : dead_entities) {
          // Verify it's still dead (Lua might have revived it in previous iteration constraint?)
          // No, entities are distinct.
          handle_death(entity);
      }
  } else {
      // Fallback: Just kill them to prevent zombie loop if script fails
      for (auto entity : dead_entities) {
          registry.KillEntity(entity);
      }
  }
}

}  // namespace game_logic::systems
