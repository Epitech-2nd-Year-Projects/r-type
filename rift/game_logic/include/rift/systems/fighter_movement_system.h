#ifndef RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_
#define RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift::systems {

class FighterMovementSystem : public engine::ecs::ISystem {
 public:
  static constexpr float kArenaWidth = 800.0f;
  static constexpr float kMinDistance = 20.0f;

  FighterMovementSystem() = default;
  ~FighterMovementSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_
