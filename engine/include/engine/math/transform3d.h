#ifndef ENGINE_MATH_TRANSFORM3D_H_
#define ENGINE_MATH_TRANSFORM3D_H_

#include "vector3.h"

namespace engine::math {

/**
 * @brief 3D transformation with position, rotation (Euler angles), and scale.
 *
 * Rotation is stored as Euler angles in degrees (pitch, yaw, roll).
 * The rotation order is YXZ (yaw -> pitch -> roll).
 */
class Transform3D {
 public:
  Transform3D();
  explicit Transform3D(const Vector3f& position);
  Transform3D(const Vector3f& position, const Vector3f& rotation,
              const Vector3f& scale);

  /**
   * @brief Get the position.
   */
  const Vector3f& GetPosition() const noexcept { return position_; }

  /**
   * @brief Set the position.
   */
  void SetPosition(const Vector3f& position) noexcept { position_ = position; }

  /**
   * @brief Translate by a delta.
   */
  void Translate(const Vector3f& delta) noexcept { position_ += delta; }

  /**
   * @brief Get rotation as Euler angles in degrees (pitch, yaw, roll).
   */
  const Vector3f& GetRotation() const noexcept { return rotation_; }

  /**
   * @brief Set rotation as Euler angles in degrees (pitch, yaw, roll).
   */
  void SetRotation(const Vector3f& euler_degrees) noexcept {
    rotation_ = euler_degrees;
  }

  /**
   * @brief Rotate by delta Euler angles in degrees.
   */
  void Rotate(const Vector3f& delta_degrees) noexcept {
    rotation_ += delta_degrees;
  }

  /**
   * @brief Set the yaw (Y-axis rotation) in degrees.
   */
  void SetYaw(float degrees) noexcept { rotation_.y = degrees; }

  /**
   * @brief Get the yaw (Y-axis rotation) in degrees.
   */
  float GetYaw() const noexcept { return rotation_.y; }

  /**
   * @brief Set the pitch (X-axis rotation) in degrees.
   */
  void SetPitch(float degrees) noexcept { rotation_.x = degrees; }

  /**
   * @brief Get the pitch (X-axis rotation) in degrees.
   */
  float GetPitch() const noexcept { return rotation_.x; }

  /**
   * @brief Set the roll (Z-axis rotation) in degrees.
   */
  void SetRoll(float degrees) noexcept { rotation_.z = degrees; }

  /**
   * @brief Get the roll (Z-axis rotation) in degrees.
   */
  float GetRoll() const noexcept { return rotation_.z; }

  /**
   * @brief Get the scale.
   */
  const Vector3f& GetScale() const noexcept { return scale_; }

  /**
   * @brief Set the scale.
   */
  void SetScale(const Vector3f& scale) noexcept { scale_ = scale; }

  /**
   * @brief Set uniform scale.
   */
  void SetScale(float uniform_scale) noexcept {
    scale_ = Vector3f(uniform_scale, uniform_scale, uniform_scale);
  }

  /**
   * @brief Get the forward direction vector based on rotation.
   */
  Vector3f Forward() const noexcept;

  /**
   * @brief Get the right direction vector based on rotation.
   */
  Vector3f Right() const noexcept;

  /**
   * @brief Get the up direction vector based on rotation.
   */
  Vector3f Up() const noexcept;

 private:
  Vector3f position_{0.0f, 0.0f, 0.0f};
  Vector3f rotation_{0.0f, 0.0f, 0.0f};
  Vector3f scale_{1.0f, 1.0f, 1.0f};
};

}  // namespace engine::math

#endif  // ENGINE_MATH_TRANSFORM3D_H_
