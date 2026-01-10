#ifndef ENGINE_RENDER_LIGHT_H_
#define ENGINE_RENDER_LIGHT_H_

#include "color.h"
#include "engine/math/vector3.h"

namespace engine::render {

/**
 * @brief Ambient light affecting all surfaces equally.
 *
 * Provides a base level of illumination to prevent completely dark areas.
 */
struct AmbientLight {
  /// @brief Light color.
  Color color{Color::White()};

  /// @brief Light intensity multiplier (0.0 to 1.0 typical).
  float intensity{0.2f};
};

/**
 * @brief Directional light simulating distant sources like the sun.
 *
 * Has no position, only a direction. All rays are parallel.
 */
struct DirectionalLight {
  /// @brief Direction the light is pointing (will be normalized).
  math::Vector3f direction{0.0f, -1.0f, -0.5f};

  /// @brief Light color.
  Color color{Color::White()};

  /// @brief Light intensity multiplier.
  float intensity{1.0f};

  /// @brief Whether the light is active.
  bool enabled{true};
};

/**
 * @brief Combined lighting configuration for a scene.
 *
 * Provides both ambient and directional lighting for basic illumination.
 */
struct LightingConfig {
  /// @brief Ambient light for base illumination.
  AmbientLight ambient;

  /// @brief Primary directional light (sun-like).
  DirectionalLight directional;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_LIGHT_H_
