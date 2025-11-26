#ifndef ENGINE_ECS_COMPONENTS_CIRCLE_COLLIDER_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_CIRCLE_COLLIDER_COMPONENT_H_

#include "engine/math/vector2.h"

namespace engine::ecs {

/**
 * @brief Circular collision area
 *
 * @details
 * Defines a circle centered on entity position with given radius.
 * Used by CircleCollisionSystem for collision detection.
 */
struct CircleColliderComponent {
  float radius{1.0f};
  math::Vector2f offset{0.0f, 0.0f};
  bool is_trigger{false};

  CircleColliderComponent() = default;
  explicit CircleColliderComponent(float r) : radius(r) {}
  CircleColliderComponent(float r, const math::Vector2f& off)
      : radius(r), offset(off) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_CIRCLE_COLLIDER_COMPONENT_H_
