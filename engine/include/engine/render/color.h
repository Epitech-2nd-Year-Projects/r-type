#ifndef ENGINE_RENDER_COLOR_H_
#define ENGINE_RENDER_COLOR_H_

#include <algorithm>
#include <cstdint>

namespace engine::render {

/**
 * @brief Normalized RGBA color helper.
 */
struct Color {
  float r{0.0f};
  float g{0.0f};
  float b{0.0f};
  float a{1.0f};

  constexpr Color() = default;
  constexpr Color(float r, float g, float b, float a = 1.0f)
      : r(r), g(g), b(b), a(a) {}

  /**
   * @brief Create color from 0-255 components.
   */
  static constexpr Color FromBytes(uint8_t r, uint8_t g, uint8_t b,
                                   uint8_t a = 255) {
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
  }

  static constexpr Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
  static constexpr Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
  static constexpr Color Transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }

  /**
   * @brief Return color with modified alpha.
   */
  constexpr Color WithAlpha(float alpha) const {
    return Color(r, g, b, std::clamp(alpha, 0.0f, 1.0f));
  }

  bool operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
  }

  bool operator!=(const Color& other) const { return !(*this == other); }
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_COLOR_H_
