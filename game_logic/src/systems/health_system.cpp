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

  std::vector<engine::ecs::EntityId> entities_to_kill;

  for (auto [entity_idx, hp] : engine::ecs::IndexedZipper(healths)) {
    if (hp->is_alive()) {
      continue;
    }

    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);
    entities_to_kill.push_back(entity);
  }

  for (auto entity : entities_to_kill) {
    registry.KillEntity(entity);
  }
}

}  // namespace game_logic::systems
