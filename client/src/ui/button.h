#ifndef CLIENT_UI_BUTTON_H_
#define CLIENT_UI_BUTTON_H_

#include <string>
#include <functional>
#include <memory>

#include "ui_element.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/render/color.h"

namespace client::ui {

class Button : public UIElement {
 public:
  Button(engine::math::Vector2f position, engine::math::Vector2f size, const std::string& text, std::function<void()> on_click);

  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input) override;
  void Draw(engine::render::Renderer2D& renderer) override;

  void SetTexture(std::shared_ptr<engine::render::Texture2D> texture);
  void SetColors(engine::render::Color normal, engine::render::Color hover, engine::render::Color pressed);
  
  const std::string& GetText() const { return text_; }
  void SetText(const std::string& text) { text_ = text; }

  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }
  engine::math::Vector2f GetPosition() const override { return position_; }
  
  void SetSize(engine::math::Vector2f size) override { size_ = size; }
  engine::math::Vector2f GetSize() const override { return size_; }

 private:
  engine::math::Vector2f position_;
  engine::math::Vector2f size_;
  std::string text_;
  std::function<void()> on_click_;
  
  bool hovered_{false};
  bool pressed_{false};
  
  std::shared_ptr<engine::render::Texture2D> texture_;
  
  engine::render::Color normal_color_{engine::render::Color::FromBytes(128, 128, 128)};
  engine::render::Color hover_color_{engine::render::Color::White()};
  engine::render::Color pressed_color_{engine::render::Color::FromBytes(180, 180, 180)};
  engine::render::Color text_color_{engine::render::Color::Black()};
};

}  // namespace client::ui

#endif // CLIENT_UI_BUTTON_H_
