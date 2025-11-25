#ifndef ENGINE_MATH_CONSTANTS_H_
#define ENGINE_MATH_CONSTANTS_H_

#include <cmath>

namespace engine::math {

/**
 * @brief Mathematical constants for the math module.
 */

/// @brief Pi constant (180 degrees in radians)
constexpr float kPi = 3.14159265359f;

/// @brief Epsilon for floating-point comparisons and zero checks
constexpr float kEpsilon = 0.0001f;

/// @brief Degrees to radians conversion factor
constexpr float kDegToRad = kPi / 180.0f;

/// @brief Radians to degrees conversion factor
constexpr float kRadToDeg = 180.0f / kPi;

/// @brief Large value approximating positive infinity
constexpr float kInfinityPositive = 1e9f;

/// @brief Large value approximating negative infinity
constexpr float kInfinityNegative = -1e9f;

}  // namespace engine::math

#endif  // ENGINE_MATH_CONSTANTS_H_