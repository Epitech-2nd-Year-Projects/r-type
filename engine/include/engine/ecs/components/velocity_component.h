#ifndef ENGINE_ECS_COMPONENTS_VELOCITY_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_VELOCITY_COMPONENT_H_

#include "engine/math/vector2.h"

namespace engine::ecs {

/**
 * @brief 2D velocity component for moving entities
 *
 * @details
 * Represents movement speed and direction. Used by MovementSystem
 * to update PositionComponent each frame.
 */
struct VelocityComponent {
  math::Vector2f velocity{0.0f, 0.0f};

  VelocityComponent() = default;
  VelocityComponent(float vx, float vy) : velocity(vx, vy) {}
  explicit VelocityComponent(const math::Vector2f& vel) : velocity(vel) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_VELOCITY_COMPONENT_H_
