/**
 * @file text_input_h
 * @brief Text input widget
 *
 * @details
 * Provides editable text fields with focus and cursor blink
 */

#ifndef ENGINE_UI_TEXT_INPUT_H_
#define ENGINE_UI_TEXT_INPUT_H_

#include <string>
#include <vector>

#include "engine/render/color.h"
#include "engine/ui/widget.h"

namespace engine::ui {

/**
 * @brief Editable text input field
 */
class TextInput : public Widget {
 public:
  /**
   * @brief Create a text input
   * @param position Initial position
   * @param size Initial size
   */
  TextInput(engine::math::Vector2f position, engine::math::Vector2f size);

  /**
   * @brief Update input state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt,
              engine::input::InputManager& input) override;

  /**
   * @brief Draw the input field
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Set text content
   * @param text Input text
   */
  void SetText(const std::string& text);

  /**
   * @brief Get text content
   * @return Current text
   */
  const std::string& GetText() const;

  /**
   * @brief Set placeholder text
   * @param placeholder Placeholder string
   */
  void SetPlaceholder(const std::string& placeholder);

  /**
   * @brief Check focus state
   * @return True when focused
   */
  bool IsFocused() const { return focused_; }

  /**
   * @brief Update focus state
   * @param focused Focus state
   */
  void SetFocused(bool focused);

  /**
   * @brief Set background color
   * @param color Background color
   */
  void SetBackgroundColor(engine::render::Color color) {
    background_color_ = color;
  }

  /**
   * @brief Set text color
   * @param color Text color
   */
  void SetTextColor(engine::render::Color color) { text_color_ = color; }

  /**
   * @brief Set input position
   * @param pos Position in screen space
   */
  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }

  /**
   * @brief Get input position
   * @return Position in screen space
   */
  engine::math::Vector2f GetPosition() const override { return position_; }

  /**
   * @brief Set input size
   * @param size Size in screen space
   */
  void SetSize(engine::math::Vector2f size) override { size_ = size; }

  /**
   * @brief Get input size
   * @return Size in screen space
   */
  engine::math::Vector2f GetSize() const override { return size_; }

 private:
  void HandleTyping(engine::input::InputManager& input);

  engine::math::Vector2f position_;
  engine::math::Vector2f size_;
  std::string text_;
  std::string placeholder_;
  bool focused_{false};
  float blink_timer_{0.0f};
  bool show_cursor_{false};

  engine::render::Color background_color_{
      engine::render::Color::FromBytes(50, 50, 50)};
  engine::render::Color text_color_{engine::render::Color::White()};
  engine::render::Color border_color_{
      engine::render::Color::FromBytes(128, 128, 128)};
  engine::render::Color focused_border_color_{
      engine::render::Color::FromBytes(100, 200, 255)};

  std::vector<bool> last_key_states_;
};

}  // namespace engine::ui

#endif  // ENGINE_UI_TEXT_INPUT_H_
