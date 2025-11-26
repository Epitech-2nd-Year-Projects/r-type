#ifndef ENGINE_ECS_SYSTEMS_AABB_COLLISION_SYSTEM_H_
#define ENGINE_ECS_SYSTEMS_AABB_COLLISION_SYSTEM_H_

#include <functional>
#include <utility>
#include <vector>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/math/collision.h"
#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Collision pair
 */
using CollisionPair = std::pair<EntityId, EntityId>;

/**
 * @brief Collision event callback signature
 */
using CollisionCallback =
    std::function<void(EntityId, EntityId, const math::CollisionInfo&)>;

/**
 * @brief AABB collision detection system
 *
 * @details
 * Detects collisions between entities with PositionComponent and
 * BoundingBoxComponent using axis-aligned bounding box tests.
 *
 * Provides callbacks for collision events.
 */
class AABBCollisionSystem : public ISystem {
 public:
  void Update(Registry& registry, time::TimeDelta dt) override;

  /**
   * @brief Set callback for collision events
   */
  void SetCollisionCallback(CollisionCallback callback);

 private:
  CollisionCallback on_collision_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEMS_AABB_COLLISION_SYSTEM_H_
