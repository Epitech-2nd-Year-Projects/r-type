#ifndef GAME_LOGIC_SYSTEMS_POWERUP_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_POWERUP_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

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
