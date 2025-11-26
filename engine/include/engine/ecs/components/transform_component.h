#ifndef ENGINE_ECS_COMPONENTS_TRANSFORM_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_TRANSFORM_COMPONENT_H_

#include "engine/math/transform.h"

namespace engine::ecs {

/**
 * @brief Complete 2D transformation (position, rotation, scale)
 *
 * @details
 * Uses engine::math::Transform for full spatial representation.
 * Prefer this over PositionComponent when rotation/scale is needed.
 */
struct TransformComponent {
  math::Transform transform;

  TransformComponent() = default;
  explicit TransformComponent(const math::Transform& t) : transform(t) {}
  TransformComponent(const math::Vector2f& pos, float rot,
                     const math::Vector2f& scale)
      : transform(pos, rot, scale) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_TRANSFORM_COMPONENT_H_