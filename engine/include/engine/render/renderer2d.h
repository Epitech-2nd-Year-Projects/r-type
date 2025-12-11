#ifndef ENGINE_RENDER_RENDERER2D_H_
#define ENGINE_RENDER_RENDERER2D_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "color.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/layer.h"

#if defined(DrawText)
#undef DrawText
#endif

namespace engine::render {

/**
 * @brief Handle for GPU 2D texture.
 */
class Texture2D {
 public:
  virtual ~Texture2D() = default;
  virtual math::Vector2i GetSize() const = 0;
};

/**
 * @brief Parameters describing how to draw a textured sprite.
 */
struct SpriteDrawParams {
  math::Vector2f position{};
  math::Vector2f origin{};
  math::Vector2f scale{1.0f, 1.0f};
  float rotation{0.0f};
  std::optional<math::RectF> source{};
  Color tint{Color::White()};
  RenderLayer layer{RenderLayer::kMidground};
};

/**
 * @brief Generic 2D renderer API.
 *
 * Allows drawing primitives and textured sprites independently from the
 * underlying graphics library.
 */
class Renderer2D {
 public:
  virtual ~Renderer2D() = default;

  virtual void DrawRect(const math::RectF& rect, const Color& color) = 0;
  virtual void DrawCircle(const math::Vector2f& center, float radius,
                          const Color& color) = 0;
  virtual void DrawLine(const math::Vector2f& start, const math::Vector2f& end,
                        float thickness, const Color& color) = 0;

  virtual void DrawTexture(const Texture2D& texture,
                           const SpriteDrawParams& params) = 0;

  virtual void DrawText(std::string_view text, const math::Vector2f& position,
                        float font_size, const Color& color) = 0;

  /**
   * @brief Measure the dimensions of a text string with the current font.
   */
  virtual math::Vector2f MeasureText(std::string_view text,
                                     float font_size) = 0;

  /**
   * @brief Load texture resource.
   */
  virtual std::shared_ptr<Texture2D> LoadTextureFromFile(
      const std::string& path) = 0;

  /**
   * @brief Load a font from a file.
   * @param name Unique identifier for the font.
   * @param path Path to the font file.
   */
  virtual void LoadFont(const std::string& name, const std::string& path) = 0;

  /**
   * @brief Set the current font to be used by DrawText.
   * @param name The identifier of the font to use.
   */
  virtual void SetFont(const std::string& name) = 0;

  /**
   * @brief Flush any pending batched draw calls.
   */
  virtual void Flush() = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_RENDERER2D_H_
