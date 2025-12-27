/**
 * @file label_h
 * @brief Text label widget
 *
 * @details
 * Provides lightweight text rendering without input handling
 */

#ifndef ENGINE_UI_LABEL_H_
#define ENGINE_UI_LABEL_H_

#include <string>

#include "engine/render/color.h"
#include "engine/ui/widget.h"

namespace engine::ui {

/**
 * @brief Simple text label widget
 */
class Label : public Widget {
 public:
  /**
   * @brief Create a label
   * @param position Initial position
   * @param text Label content
   * @param font_size Font size in pixels
   * @param color Text color
   */
  Label(engine::math::Vector2f position, const std::string& text,
        float font_size,
        engine::render::Color color = engine::render::Color::White());

  /**
   * @brief Update label state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt,
              engine::input::InputManager& input) override;

  /**
   * @brief Draw the label
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Set label text
   * @param text Label content
   */
  void SetText(const std::string& text);

  /**
   * @brief Set label position
   * @param pos Position in screen space
   */
  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }

  /**
   * @brief Get label position
   * @return Position in screen space
   */
  engine::math::Vector2f GetPosition() const override { return position_; }

  /**
   * @brief Set label size
   * @param size Size in screen space
   */
  void SetSize(engine::math::Vector2f size) override { size_ = size; }

  /**
   * @brief Get label size
   * @return Size in screen space
   */
  engine::math::Vector2f GetSize() const override { return size_; }

 private:
  engine::math::Vector2f position_;
  engine::math::Vector2f size_{0.0f, 0.0f};
  std::string text_;
  float font_size_;
  engine::render::Color color_;
};

}  // namespace engine::ui

#endif  // ENGINE_UI_LABEL_H_
