#ifndef ENGINE_ECS_SYSTEMS_MOVEMENT_SYSTEM_H_
#define ENGINE_ECS_SYSTEMS_MOVEMENT_SYSTEM_H_

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Applies velocity to position each frame
 *
 * @details
 * Iterates over entities with both PositionComponent and VelocityComponent,
 * updating position by velocity * delta_time.
 *
 * Typically runs as Fixed system for physics consistency.
 */
class MovementSystem {
 public:
  static void Update(Registry& registry,
                     SparseArray<PositionComponent>& positions,
                     SparseArray<VelocityComponent>& velocities,
                     time::TimeDelta dt);
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEMS_MOVEMENT_SYSTEM_H_
