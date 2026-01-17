#ifndef GAME_LOGIC_SYSTEMS_PLAYER_INPUT_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_PLAYER_INPUT_SYSTEM_H_

#include <functional>

#include "engine/ecs/component.h"
#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"
#include "game_logic/components.h"

namespace game_logic {

class GameInstance;

namespace systems {

/**
 * @class PlayerInputSystem
 * @brief Consumes queued player input events and updates velocity and weapon
 * triggers.
 */
class PlayerInputSystem {
 public:
  /**
   * @brief ECS system update entry point.
   *
   * @param registry ECS registry (unused)
   * @param players Player components array
   * @param velocities Velocity components array
   * @param weapons Weapon components array
   * @param dt Frame delta time
   * @param instance_ref Reference to owning GameInstance
   */
  static void Update(
      engine::ecs::Registry &registry,
      engine::ecs::SparseArray<components::PlayerComponent> &players,
      engine::ecs::SparseArray<engine::ecs::VelocityComponent> &velocities,
      engine::ecs::SparseArray<components::WeaponComponent> &weapons,
      engine::ecs::SparseArray<components::ShootEventComponent> &shoot_events,
      engine::time::TimeDelta dt,
      std::reference_wrapper<GameInstance> instance_ref);
};

}  // namespace systems
}  // namespace game_logic

#endif  // GAME_LOGIC_SYSTEMS_PLAYER_INPUT_SYSTEM_H_
