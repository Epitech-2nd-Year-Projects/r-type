#ifndef ENGINE_MATH_VECTOR2_H_
#define ENGINE_MATH_VECTOR2_H_

#include <cmath>
#include <ostream>

namespace engine::math {

/**
 * @brief 2D vector for positions, velocities, directions.
 * @tparam T Scalar type (float, double, int)
 */
template <typename T = float>
class Vector2 {
 public:
  T x;
  T y;

  Vector2() : x(0), y(0) {}
  Vector2(T x, T y) : x(x), y(y) {}
  Vector2(const Vector2& other) = default;
  Vector2& operator=(const Vector2& other) = default;

  Vector2 operator+(const Vector2& other) const {
    return Vector2(x + other.x, y + other.y);
  }

  Vector2 operator-(const Vector2& other) const {
    return Vector2(x - other.x, y - other.y);
  }

  Vector2 operator*(T scalar) const { return Vector2(x * scalar, y * scalar); }

  Vector2 operator/(T scalar) const { return Vector2(x / scalar, y / scalar); }

  Vector2& operator+=(const Vector2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  Vector2& operator-=(const Vector2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  Vector2& operator*=(T scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }

  Vector2 operator-() const { return Vector2(-x, -y); }

  bool operator==(const Vector2& other) const {
    return x == other.x && y == other.y;
  }

  bool operator!=(const Vector2& other) const { return !(*this == other); }

  /**
   * @brief Dot product.
   */
  T Dot(const Vector2& other) const { return x * other.x + y * other.y; }

  /**
   * @brief Cross product (2D returns scalar).
   */
  T Cross(const Vector2& other) const { return x * other.y - y * other.x; }

  /**
   * @brief Get length magnitude.
   */
  T Length() const { return static_cast<T>(std::sqrt(x * x + y * y)); }

  /**
   * @brief Get squared length (faster, no sqrt).
   */
  T LengthSquared() const { return x * x + y * y; }

  /**
   * @brief Distance to another point.
   */
  T Distance(const Vector2& other) const { return (*this - other).Length(); }

  /**
   * @brief Return normalized unit vector.
   */
  Vector2 Normalized() const {
    T len = Length();
    if (len == 0) return Vector2(0, 0);
    return *this / len;
  }

  /**
   * @brief Normalize this vector in-place.
   */
  void Normalize() {
    T len = Length();
    if (len != 0) {
      x /= len;
      y /= len;
    }
  }

  /**
   * @brief Return rotated vector (radians).
   */
  Vector2 Rotated(T angle_rad) const {
    T cos_a = static_cast<T>(std::cos(angle_rad));
    T sin_a = static_cast<T>(std::sin(angle_rad));
    return Vector2(x * cos_a - y * sin_a, x * sin_a + y * cos_a);
  }

  /**
   * @brief Reflect vector off normal.
   */
  Vector2 Reflected(const Vector2& normal) const {
    return *this - normal * (2 * Dot(normal));
  }

  /**
   * @brief Linear interpolation between two vectors.
   */
  static Vector2 Lerp(const Vector2& a, const Vector2& b, T t) {
    return a + (b - a) * t;
  }
};

/**
 * @brief Scalar * Vector (reverse multiplication order).
 */
template <typename T>
inline Vector2<T> operator*(T scalar, const Vector2<T>& v) {
  return v * scalar;
}

using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;

}  // namespace engine::math

#endif