/**
 * @file button_h
 * @brief Clickable button widget
 *
 * @details
 * Provides hover and pressed visuals plus click callbacks
 */

#ifndef ENGINE_UI_BUTTON_H_
#define ENGINE_UI_BUTTON_H_

#include <functional>
#include <memory>
#include <string>

#include "engine/render/color.h"
#include "engine/render/renderer2d.h"
#include "engine/ui/widget.h"

namespace engine::ui {

/**
 * @brief Interactive button widget
 */
class Button : public Widget {
 public:
  /**
   * @brief Create a button
   * @param position Initial position
   * @param size Initial size
   * @param text Button label
   * @param on_click Callback fired on click
   */
  Button(engine::math::Vector2f position, engine::math::Vector2f size,
         const std::string& text, std::function<void()> on_click);

  /**
   * @brief Update input state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt,
              engine::input::InputManager& input) override;

  /**
   * @brief Draw the button
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Assign a texture
   * @param texture Texture handle
   */
  void SetTexture(std::shared_ptr<engine::render::Texture2D> texture);

  /**
   * @brief Set color palette
   * @param normal Normal color
   * @param hover Hover color
   * @param pressed Pressed color
   */
  void SetColors(engine::render::Color normal, engine::render::Color hover,
                 engine::render::Color pressed);

  /**
   * @brief Set text color
   * @param color Text color
   */
  void SetTextColor(engine::render::Color color) { text_color_ = color; }

  /**
   * @brief Set text scale
   * @param scale Relative scale factor
   */
  void SetTextScale(float scale) { text_scale_ = scale; }

  /**
   * @brief Get current text
   * @return Text content
   */
  const std::string& GetText() const { return text_; }

  /**
   * @brief Set button text
   * @param text Text content
   */
  void SetText(const std::string& text) { text_ = text; }

  /**
   * @brief Set button position
   * @param pos Position in screen space
   */
  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }

  /**
   * @brief Get button position
   * @return Position in screen space
   */
  engine::math::Vector2f GetPosition() const override { return position_; }

  /**
   * @brief Set button size
   * @param size Size in screen space
   */
  void SetSize(engine::math::Vector2f size) override { size_ = size; }

  /**
   * @brief Get button size
   * @return Size in screen space
   */
  engine::math::Vector2f GetSize() const override { return size_; }

 private:
  engine::math::Vector2f position_;
  engine::math::Vector2f size_;
  std::string text_;
  std::function<void()> on_click_;

  bool hovered_{false};
  bool pressed_{false};

  std::shared_ptr<engine::render::Texture2D> texture_;

  engine::render::Color normal_color_{
      engine::render::Color::FromBytes(128, 128, 128)};
  engine::render::Color hover_color_{engine::render::Color::White()};
  engine::render::Color pressed_color_{
      engine::render::Color::FromBytes(180, 180, 180)};
  engine::render::Color text_color_{engine::render::Color::Black()};
  float text_scale_{0.4f};
};

}  // namespace engine::ui

#endif  // ENGINE_UI_BUTTON_H_
