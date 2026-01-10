#ifndef ENGINE_ECS_COMPONENTS_TRANSFORM3D_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_TRANSFORM3D_COMPONENT_H_

#include "engine/math/transform3d.h"

namespace engine::ecs {

/**
 * @brief Complete 3D transformation (position, rotation, scale)
 *
 * @details
 * Uses engine::math::Transform3D for full 3D spatial representation.
 * Rotation is stored as Euler angles in degrees.
 */
struct Transform3DComponent {
  math::Transform3D transform;

  Transform3DComponent() = default;
  explicit Transform3DComponent(const math::Transform3D& t) : transform(t) {}
  Transform3DComponent(const math::Vector3f& pos, const math::Vector3f& rot,
                       const math::Vector3f& scale)
      : transform(pos, rot, scale) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_TRANSFORM3D_COMPONENT_H_
