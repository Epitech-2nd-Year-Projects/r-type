#ifndef ENGINE_ECS_SYSTEMS_CIRCLE_COLLISION_SYSTEM_H_
#define ENGINE_ECS_SYSTEMS_CIRCLE_COLLISION_SYSTEM_H_

#include <functional>
#include <utility>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Circle collision event callback
 */
using CircleCollisionCallback = std::function<void(EntityId, EntityId)>;

/**
 * @brief Circle collision detection system
 *
 * @details
 * Detects collisions between entities with PositionComponent and
 * CircleColliderComponent using circle-circle distance tests.
 */
class CircleCollisionSystem : public ISystem {
 public:
  void Update(Registry& registry, time::TimeDelta dt) override;

  /**
   * @brief Set callback for collision events
   */
  void SetCollisionCallback(CircleCollisionCallback callback);

 private:
  CircleCollisionCallback on_collision_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEMS_CIRCLE_COLLISION_SYSTEM_H_
