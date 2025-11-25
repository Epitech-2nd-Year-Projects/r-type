#ifndef ENGINE_MATH_TRANSFORM_H_
#define ENGINE_MATH_TRANSFORM_H_

#include "vector2.h"

namespace engine::math {

/**
 * @brief Position, rotation, scale transformation.
 */
class Transform {
 public:
  Transform();
  explicit Transform(const Vector2f& pos);
  Transform(const Vector2f& pos, float rot, const Vector2f& scl);

  /**
   * @brief Get position.
   */
  Vector2f GetPosition() const;

  /**
   * @brief Set position.
   */
  void SetPosition(const Vector2f& pos);

  /**
   * @brief Get rotation in degrees.
   */
  float GetRotation() const;

  /**
   * @brief Set rotation in degrees.
   */
  void SetRotation(float degrees);

  /**
   * @brief Rotate by degrees.
   */
  void Rotate(float degrees);

  /**
   * @brief Rotate by radians.
   */
  void RotateRad(float radians);

  /**
   * @brief Get rotation in radians.
   */
  float GetRotationRad() const;

  /**
   * @brief Set rotation in radians.
   */
  void SetRotationRad(float rad);

  /**
   * @brief Get scale.
   */
  Vector2f GetScale() const;

  /**
   * @brief Set scale.
   */
  void SetScale(const Vector2f& scl);

 private:
  Vector2f position_;
  float rotation_;
  Vector2f scale_;
};

}  // namespace engine::math

#endif