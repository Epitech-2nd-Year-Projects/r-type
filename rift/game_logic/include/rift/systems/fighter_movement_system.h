#ifndef RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_
#define RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"
#include "rift/arena_constants.h"

namespace rift::systems {

class FighterMovementSystem : public engine::ecs::ISystem {
 public:
  static constexpr float kArenaWidth = ArenaConstants::kArenaWidth;
  static constexpr float kFighterWidth = ArenaConstants::kFighterWidth;
  static constexpr float kMinDistance = ArenaConstants::kMinDistance;
  static constexpr float kGravity = ArenaConstants::kGravity;
  static constexpr float kGroundY = ArenaConstants::kGroundY;

  FighterMovementSystem() = default;
  ~FighterMovementSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_FIGHTER_MOVEMENT_SYSTEM_H_
