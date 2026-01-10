#ifndef ENGINE_RENDER_CAMERA3D_H_
#define ENGINE_RENDER_CAMERA3D_H_

#include "engine/math/vector3.h"

namespace engine::render {

/**
 * @brief Camera projection type.
 */
enum class CameraProjection : int {
  kPerspective = 0,   ///< Perspective projection (3D depth)
  kOrthographic = 1,  ///< Orthographic projection (2D-like)
};

/**
 * @brief 3D camera supporting orbital (third-person) movement.
 *
 * The camera orbits around a target point at a specified distance.
 * Position is computed from target, distance, yaw, and pitch.
 */
class Camera3D {
 public:
  Camera3D();

  /**
   * @brief Set the point the camera looks at.
   */
  void SetTarget(const math::Vector3f& target) noexcept;

  /**
   * @brief Get the point the camera looks at.
   */
  const math::Vector3f& GetTarget() const noexcept;

  /**
   * @brief Set the distance from the target.
   */
  void SetDistance(float distance) noexcept;

  /**
   * @brief Get the distance from the target.
   */
  float GetDistance() const noexcept;

  /**
   * @brief Set the horizontal rotation angle (yaw) in degrees.
   */
  void SetYaw(float degrees) noexcept;

  /**
   * @brief Get the horizontal rotation angle (yaw) in degrees.
   */
  float GetYaw() const noexcept;

  /**
   * @brief Set the vertical rotation angle (pitch) in degrees.
   * Clamped to prevent flipping.
   */
  void SetPitch(float degrees) noexcept;

  /**
   * @brief Get the vertical rotation angle (pitch) in degrees.
   */
  float GetPitch() const noexcept;

  /**
   * @brief Rotate horizontally around the target.
   * @param delta_degrees Degrees to rotate (positive = clockwise from above).
   */
  void OrbitHorizontal(float delta_degrees) noexcept;

  /**
   * @brief Rotate vertically around the target.
   * @param delta_degrees Degrees to rotate (positive = upward).
   */
  void OrbitVertical(float delta_degrees) noexcept;

  /**
   * @brief Zoom in or out by adjusting distance.
   * @param delta Positive zooms out, negative zooms in.
   */
  void Zoom(float delta) noexcept;

  /**
   * @brief Get the camera position computed from orbital parameters.
   */
  math::Vector3f GetPosition() const noexcept;

  /**
   * @brief Get the camera's up vector.
   */
  math::Vector3f GetUp() const noexcept;

  /**
   * @brief Set the vertical field of view in degrees.
   */
  void SetFov(float degrees) noexcept;

  /**
   * @brief Get the vertical field of view in degrees.
   */
  float GetFov() const noexcept;

  /**
   * @brief Set the projection type.
   */
  void SetProjection(CameraProjection projection) noexcept;

  /**
   * @brief Get the projection type.
   */
  CameraProjection GetProjection() const noexcept;

 private:
  math::Vector3f target_{0.0f, 0.0f, 0.0f};
  float distance_{10.0f};
  float yaw_{0.0f};      ///< Horizontal angle in degrees
  float pitch_{-30.0f};  ///< Vertical angle in degrees
  float fov_{45.0f};     ///< Vertical field of view in degrees
  CameraProjection projection_{CameraProjection::kPerspective};

  static constexpr float kMinPitch = -89.0f;
  static constexpr float kMaxPitch = 89.0f;
  static constexpr float kMinDistance = 0.1f;
  static constexpr float kMaxDistance = 1000.0f;
  static constexpr float kMinFov = 1.0f;
  static constexpr float kMaxFov = 179.0f;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_CAMERA3D_H_
