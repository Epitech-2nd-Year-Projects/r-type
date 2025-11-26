#ifndef ENGINE_ECS_COMPONENTS_POSITION_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_POSITION_COMPONENT_H_

#include "engine/math/vector2.h"

namespace engine::ecs {

/**
 * @brief Simple 2D position component
 *
 * @details
 * Stores a 2D position using Vector2f. This is the minimal
 * component for spatial representation.
 */
struct PositionComponent {
  math::Vector2f position{0.0f, 0.0f};

  PositionComponent() = default;
  PositionComponent(float x, float y) : position(x, y) {}
  explicit PositionComponent(const math::Vector2f& pos) : position(pos) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_POSITION_COMPONENT_H_
