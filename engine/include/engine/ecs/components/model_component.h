#ifndef ENGINE_ECS_COMPONENTS_MODEL_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_MODEL_COMPONENT_H_

#include <cstdint>
#include <string>

namespace engine::ecs {

/**
 * @brief 3D model rendering data
 *
 * @details
 * Stores model asset path and rendering options.
 * Used by 3D rendering systems to draw entity visuals.
 */
struct ModelComponent {
  /// @brief Model asset path
  std::string model_path;

  /// @brief Visibility toggle
  bool visible{true};

  /// @brief Draw as wireframe instead of solid
  bool wireframe{false};

  /// @brief Tint color (RGBA 0-255)
  struct Tint {
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};
  } tint;

  ModelComponent() = default;
  explicit ModelComponent(std::string path) : model_path(std::move(path)) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_MODEL_COMPONENT_H_
