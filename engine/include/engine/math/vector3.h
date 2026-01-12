#ifndef ENGINE_MATH_VECTOR3_H_
#define ENGINE_MATH_VECTOR3_H_

#include <cmath>
#include <ostream>

namespace engine::math {

/**
 * @brief 3D vector for positions, velocities, directions.
 * @tparam T Scalar type (float, double, int)
 */
template <typename T = float>
class Vector3 {
 public:
  T x;
  T y;
  T z;

  Vector3() : x(0), y(0), z(0) {}
  Vector3(T x, T y, T z) : x(x), y(y), z(z) {}
  Vector3(const Vector3& other) = default;
  Vector3& operator=(const Vector3& other) = default;

  Vector3 operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
  }

  Vector3 operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
  }

  Vector3 operator*(T scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
  }

  Vector3 operator/(T scalar) const {
    return Vector3(x / scalar, y / scalar, z / scalar);
  }

  Vector3& operator+=(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  Vector3& operator-=(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  Vector3& operator*=(T scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  Vector3& operator/=(T scalar) {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }

  Vector3 operator-() const { return Vector3(-x, -y, -z); }

  bool operator==(const Vector3& other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  bool operator!=(const Vector3& other) const { return !(*this == other); }

  /**
   * @brief Dot product.
   */
  T Dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  /**
   * @brief Cross product.
   */
  Vector3 Cross(const Vector3& other) const {
    return Vector3(y * other.z - z * other.y, z * other.x - x * other.z,
                   x * other.y - y * other.x);
  }

  /**
   * @brief Get length magnitude.
   */
  T Length() const { return static_cast<T>(std::sqrt(x * x + y * y + z * z)); }

  /**
   * @brief Get squared length (faster, no sqrt).
   */
  T LengthSquared() const { return x * x + y * y + z * z; }

  /**
   * @brief Distance to another point.
   */
  T Distance(const Vector3& other) const { return (*this - other).Length(); }

  /**
   * @brief Return normalized unit vector.
   */
  Vector3 Normalized() const {
    T len = Length();
    if (len == 0) return Vector3(0, 0, 0);
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
      z /= len;
    }
  }

  /**
   * @brief Reflect vector off normal.
   */
  Vector3 Reflected(const Vector3& normal) const {
    return *this - normal * (2 * Dot(normal));
  }

  /**
   * @brief Linear interpolation between two vectors.
   */
  static Vector3 Lerp(const Vector3& a, const Vector3& b, T t) {
    return a + (b - a) * t;
  }

  /**
   * @brief World up direction (positive Y).
   */
  static Vector3 Up() { return Vector3(0, 1, 0); }

  /**
   * @brief World down direction (negative Y).
   */
  static Vector3 Down() { return Vector3(0, -1, 0); }

  /**
   * @brief World forward direction (negative Z in right-handed system).
   */
  static Vector3 Forward() { return Vector3(0, 0, -1); }

  /**
   * @brief World backward direction (positive Z in right-handed system).
   */
  static Vector3 Back() { return Vector3(0, 0, 1); }

  /**
   * @brief World right direction (positive X).
   */
  static Vector3 Right() { return Vector3(1, 0, 0); }

  /**
   * @brief World left direction (negative X).
   */
  static Vector3 Left() { return Vector3(-1, 0, 0); }

  /**
   * @brief Zero vector.
   */
  static Vector3 Zero() { return Vector3(0, 0, 0); }

  /**
   * @brief One vector (1, 1, 1).
   */
  static Vector3 One() { return Vector3(1, 1, 1); }
};

/**
 * @brief Scalar * Vector (reverse multiplication order).
 */
template <typename T>
inline Vector3<T> operator*(T scalar, const Vector3<T>& v) {
  return v * scalar;
}

using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
using Vector3i = Vector3<int>;

}  // namespace engine::math

#endif  // ENGINE_MATH_VECTOR3_H_
