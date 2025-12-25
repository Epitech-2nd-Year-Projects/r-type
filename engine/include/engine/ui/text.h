/**
 * @file text.h
 * @brief Text rendering element for UI layouts
 *
 * @details
 * Provides a lightweight text block that measures itself through the renderer
 * to enable responsive alignment and stacking without manual coordinates.
 */

#ifndef ENGINE_UI_TEXT_H_
#define ENGINE_UI_TEXT_H_

#include <optional>
#include <string>

#include "engine/render/color.h"
#include "engine/ui/element.h"

namespace engine::ui {

/**
 * @brief Text element that integrates with the layout system
 */
class TextElement : public UIElement {
 public:
  TextElement(std::string text, FontSize font_size, render::Color color);

  /**
   * @brief Update displayed text
   */
  void SetText(std::string text);

  /**
   * @brief Update font sizing rule
   */
  void SetFontSize(FontSize size);

  /**
   * @brief Set the font used to render this text
   */
  void SetFont(std::string font_name);

  /**
   * @brief Set the font restored after measuring and drawing when a custom font
   * is used
   * @details If empty the renderer keeps the custom font after measure and draw
   */
  void SetFontFallback(std::string font_name);

  /**
   * @brief Update text color
   */
  void SetColor(render::Color color) { color_ = color; }

 protected:
  math::Vector2f ComputeContentSize(
      const LayoutContext& context,
      const math::Vector2f& available_space) override;

  void Draw(render::Renderer2D& renderer) const override;

 private:
  std::string text_;
  FontSize font_size_;
  render::Color color_;
  float resolved_font_size_{0.0f};
  std::optional<std::string> font_name_{};
  std::optional<std::string> fallback_font_name_{};
};

}  // namespace engine::ui

#endif  // ENGINE_UI_TEXT_H_
