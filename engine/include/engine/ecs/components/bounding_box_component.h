#ifndef ENGINE_ECS_COMPONENTS_BOUNDING_BOX_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_BOUNDING_BOX_COMPONENT_H_

#include "engine/math/rect.h"

namespace engine::ecs {

/**
 * @brief Axis-Aligned Bounding Box collider
 *
 * @details
 * Defines a rectangular collision area relative to entity position.
 * Used by AABBCollisionSystem for collision detection.
 */
struct BoundingBoxComponent {
  math::RectF bounds;
  bool is_trigger{false};

  BoundingBoxComponent() = default;
  BoundingBoxComponent(float x, float y, float w, float h)
      : bounds(x, y, w, h) {}
  explicit BoundingBoxComponent(const math::RectF& rect) : bounds(rect) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_BOUNDING_BOX_COMPONENT_H_
