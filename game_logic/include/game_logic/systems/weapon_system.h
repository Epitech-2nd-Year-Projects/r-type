#ifndef GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_

#include "engine/ecs/component.h"
#include "engine/ecs/registry.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/time/time_delta.h"
#include "game_logic/components.h"

namespace game_logic::systems {

/**
 * @class WeaponSystem
 * @brief Handles weapon cooldowns and projectile spawning.
 */
class WeaponSystem {
 public:
  /**
   * @brief ECS system update entry point.
   *
   * @param registry ECS registry
   * @param positions Position components array
   * @param weapons Weapon components array
   * @param sprites Sprite components array
   * @param dt Frame delta time
   */
  static void Update(
      engine::ecs::Registry &registry,
      engine::ecs::SparseArray<engine::ecs::PositionComponent> &positions,
      engine::ecs::SparseArray<game_logic::components::WeaponComponent>
          &weapons,
      engine::ecs::SparseArray<game_logic::components::SpriteComponent>
          &sprites,
      engine::time::TimeDelta dt,
      engine::scripting::PrefabFactory &prefab_factory);
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_
