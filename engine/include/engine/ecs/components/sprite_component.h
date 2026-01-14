#ifndef ENGINE_ECS_COMPONENTS_SPRITE_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_SPRITE_COMPONENT_H_

#include <cstdint>
#include <string>

#include "engine/math/rect.h"

namespace engine::ecs {

/**
 * @brief Sprite rendering data
 *
 * @details
 * Stores texture path/ID and source rectangle for sprite sheets.
 * Rendering system uses this to draw entity visuals.
 */
struct SpriteComponent {
  /// @brief Texture asset path or ID
  std::string texture_path;

  /// @brief Source rectangle in sprite sheet (pixels)
  math::RectF source_rect{0.0f, 0.0f, 32.0f, 32.0f};

  /// @brief Render layer (higher = drawn on top)
  std::uint8_t layer{0};

  /// @brief Visibility toggle
  bool visible{true};

  /// @brief Flip sprite horizontally
  bool flip_x{false};

  /// @brief Flip sprite vertically
  bool flip_y{false};

  /// @brief Tint color (RGBA 0-255)
  struct Tint {
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};
  } tint;

  SpriteComponent() = default;
  explicit SpriteComponent(std::string path) : texture_path(std::move(path)) {}
  SpriteComponent(std::string path, const math::RectF& rect)
      : texture_path(std::move(path)), source_rect(rect) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_SPRITE_COMPONENT_H_
