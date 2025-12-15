#ifndef GAME_LOGIC_SYSTEMS_POWERUP_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_POWERUP_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class PowerupSystem
 * @brief System responsible for updating and handling powerups.
 *
 * @details
 * This system manages powerup entities, checks for collisions with players,
 * handles powerup collection effects, and cleans up off-screen powerups.
 */
class PowerupSystem : public engine::ecs::ISystem {
 public:
  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;

 private:
  void CollectPowerup(engine::ecs::Registry &registry,
                      engine::ecs::EntityId player,
                      engine::ecs::EntityId powerup);
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_POWERUP_SYSTEM_H_
