#ifndef ENGINE_ECS_COMPONENTS_COMPOUND_CIRCLE_COLLIDER_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_COMPOUND_CIRCLE_COLLIDER_COMPONENT_H_

#include <vector>

#include "engine/math/vector2.h"

namespace engine::ecs {

/**
 * @brief Definition of a single circle within a compound collider
 *
 * @details
 * Each circle has a radius and an offset relative to the entity's position.
 */
struct CircleDefinition {
  float radius{1.0f};
  math::Vector2f offset{0.0f, 0.0f};

  CircleDefinition() = default;
  CircleDefinition(float r, float off_x, float off_y)
      : radius(r), offset(off_x, off_y) {}
  CircleDefinition(float r, const math::Vector2f &off)
      : radius(r), offset(off) {}
};

/**
 * @brief Compound collider made of multiple circles
 *
 * @details
 * Allows complex shapes to be approximated by multiple overlapping circles.
 * Each circle has its own radius and offset relative to the entity's position.
 * Used for entities with irregular shapes that don't fit well in a single
 * bounding box (e.g., the Dobkeratops boss).
 */
struct CompoundCircleColliderComponent {
  std::vector<CircleDefinition> circles;
  bool is_trigger{false};

  CompoundCircleColliderComponent() = default;
  explicit CompoundCircleColliderComponent(std::vector<CircleDefinition> c)
      : circles(std::move(c)) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_COMPOUND_CIRCLE_COLLIDER_COMPONENT_H_
