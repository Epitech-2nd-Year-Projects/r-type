#ifndef CLIENT_UI_TEXT_INPUT_H_
#define CLIENT_UI_TEXT_INPUT_H_

#include <string>
#include <vector>

#include "ui_element.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"

namespace client::ui {

class TextInput : public UIElement {
 public:
  TextInput(engine::math::Vector2f position, engine::math::Vector2f size);

  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input) override;
  void Draw(engine::render::Renderer2D& renderer) override;

  void SetText(const std::string& text);
  const std::string& GetText() const;

  void SetPlaceholder(const std::string& placeholder);
  
  bool IsFocused() const { return focused_; }
  void SetFocused(bool focused);

  void SetBackgroundColor(engine::render::Color color) { background_color_ = color; }
  void SetTextColor(engine::render::Color color) { text_color_ = color; }

  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }
  engine::math::Vector2f GetPosition() const override { return position_; }
  
  void SetSize(engine::math::Vector2f size) override { size_ = size; }
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

  engine::render::Color background_color_{engine::render::Color::FromBytes(50, 50, 50)};
  engine::render::Color text_color_{engine::render::Color::White()};
  engine::render::Color border_color_{engine::render::Color::FromBytes(128, 128, 128)};
  engine::render::Color focused_border_color_{engine::render::Color::FromBytes(100, 200, 255)};

  std::vector<bool> last_key_states_;
};

}  // namespace client::ui

#endif // CLIENT_UI_TEXT_INPUT_H_
