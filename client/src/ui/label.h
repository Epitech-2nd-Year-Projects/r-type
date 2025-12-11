#ifndef CLIENT_UI_LABEL_H_
#define CLIENT_UI_LABEL_H_

#include <string>
#include "ui_element.h"
#include "engine/render/renderer2d.h"
#include "engine/render/color.h"

namespace client::ui {

class Label : public UIElement {
 public:
  Label(engine::math::Vector2f position, const std::string& text, float font_size, engine::render::Color color = engine::render::Color::White());

  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  
  void SetText(const std::string& text);

  void SetPosition(engine::math::Vector2f pos) override { position_ = pos; }
  engine::math::Vector2f GetPosition() const override { return position_; }
  
  void SetSize(engine::math::Vector2f size) override { size_ = size; }
  engine::math::Vector2f GetSize() const override { return size_; }

 private:
  engine::math::Vector2f position_;
  engine::math::Vector2f size_{0.0f, 0.0f};
  std::string text_;
  float font_size_;
  engine::render::Color color_;
};

} // namespace client::ui

#endif // CLIENT_UI_LABEL_H_
