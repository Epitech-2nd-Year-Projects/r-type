#include "engine/render/camera3d.h"

#include <algorithm>
#include <cmath>

#include "engine/math/constants.h"

namespace engine::render {

Camera3D::Camera3D() = default;

void Camera3D::SetTarget(const math::Vector3f& target) noexcept {
  target_ = target;
}

const math::Vector3f& Camera3D::GetTarget() const noexcept { return target_; }

void Camera3D::SetDistance(float distance) noexcept {
  distance_ = std::clamp(distance, kMinDistance, kMaxDistance);
}

float Camera3D::GetDistance() const noexcept { return distance_; }

void Camera3D::SetYaw(float degrees) noexcept { yaw_ = degrees; }

float Camera3D::GetYaw() const noexcept { return yaw_; }

void Camera3D::SetPitch(float degrees) noexcept {
  pitch_ = std::clamp(degrees, kMinPitch, kMaxPitch);
}

float Camera3D::GetPitch() const noexcept { return pitch_; }

void Camera3D::OrbitHorizontal(float delta_degrees) noexcept {
  yaw_ += delta_degrees;
  while (yaw_ > 360.0f) yaw_ -= 360.0f;
  while (yaw_ < -360.0f) yaw_ += 360.0f;
}

void Camera3D::OrbitVertical(float delta_degrees) noexcept {
  SetPitch(pitch_ + delta_degrees);
}

void Camera3D::Zoom(float delta) noexcept { SetDistance(distance_ + delta); }

math::Vector3f Camera3D::GetPosition() const noexcept {
  const float yaw_rad = yaw_ * math::kDegToRad;
  const float pitch_rad = pitch_ * math::kDegToRad;

  const float cos_pitch = std::cos(pitch_rad);
  const float sin_pitch = std::sin(pitch_rad);
  const float cos_yaw = std::cos(yaw_rad);
  const float sin_yaw = std::sin(yaw_rad);

  const float x = distance_ * cos_pitch * sin_yaw;
  const float y = distance_ * -sin_pitch;
  const float z = distance_ * cos_pitch * cos_yaw;

  return target_ + math::Vector3f(x, y, z);
}

math::Vector3f Camera3D::GetUp() const noexcept { return math::Vector3f::Up(); }

void Camera3D::SetFov(float degrees) noexcept {
  fov_ = std::clamp(degrees, kMinFov, kMaxFov);
}

float Camera3D::GetFov() const noexcept { return fov_; }

void Camera3D::SetProjection(CameraProjection projection) noexcept {
  projection_ = projection;
}

CameraProjection Camera3D::GetProjection() const noexcept {
  return projection_;
}

}  // namespace engine::render
