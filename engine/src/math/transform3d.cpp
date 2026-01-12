#include "engine/math/transform3d.h"

#include <cmath>

#include "engine/math/constants.h"

namespace engine::math {

Transform3D::Transform3D()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(0.0f, 0.0f, 0.0f),
      scale_(1.0f, 1.0f, 1.0f) {}

Transform3D::Transform3D(const Vector3f& position)
    : position_(position),
      rotation_(0.0f, 0.0f, 0.0f),
      scale_(1.0f, 1.0f, 1.0f) {}

Transform3D::Transform3D(const Vector3f& position, const Vector3f& rotation,
                         const Vector3f& scale)
    : position_(position), rotation_(rotation), scale_(scale) {}

Vector3f Transform3D::Forward() const noexcept {
  const float pitch_rad = rotation_.x * kDegToRad;
  const float yaw_rad = rotation_.y * kDegToRad;

  const float cos_pitch = std::cos(pitch_rad);
  const float sin_pitch = std::sin(pitch_rad);
  const float cos_yaw = std::cos(yaw_rad);
  const float sin_yaw = std::sin(yaw_rad);

  return Vector3f(-sin_yaw * cos_pitch, sin_pitch, -cos_yaw * cos_pitch)
      .Normalized();
}

Vector3f Transform3D::Right() const noexcept {
  const float yaw_rad = rotation_.y * kDegToRad;
  const float cos_yaw = std::cos(yaw_rad);
  const float sin_yaw = std::sin(yaw_rad);

  return Vector3f(cos_yaw, 0.0f, -sin_yaw);
}

Vector3f Transform3D::Up() const noexcept {
  return Right().Cross(Forward()).Normalized();
}

}  // namespace engine::math
