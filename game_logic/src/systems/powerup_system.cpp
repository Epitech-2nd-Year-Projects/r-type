#include "game_logic/systems/powerup_system.h"

#include <vector>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/powerup_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

void PowerupSystem::Update(engine::ecs::Registry &registry,
                           engine::time::TimeDelta dt) {
  auto &powerups = registry.GetComponents<components::PowerupComponent>();
  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();

  std::vector<engine::ecs::EntityId> to_remove;
  const float kCleanupX = -100.0f;

  for (auto [entity, powerup, pos] :
       engine::ecs::IndexedZipper(powerups, positions)) {
    if (pos->position.x < kCleanupX) {
      to_remove.push_back(registry.EntityFromIndex(entity));
    }
  }

  for (auto e : to_remove) {
    registry.KillEntity(e);
  }
}

void PowerupSystem::CollectPowerup(engine::ecs::Registry &registry,
                                   engine::ecs::EntityId player,
                                   engine::ecs::EntityId powerup) {}

}  // namespace game_logic::systems
