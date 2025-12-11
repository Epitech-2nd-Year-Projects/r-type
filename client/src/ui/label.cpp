#include "label.h"

namespace client::ui {

Label::Label(engine::math::Vector2f position, const std::string& text,
             float font_size, engine::render::Color color)
    : position_(position), text_(text), font_size_(font_size), color_(color) {}

void Label::Update(engine::time::TimeDelta, engine::input::InputManager&) {}

void Label::Draw(engine::render::Renderer2D& renderer) {
  size_ = renderer.MeasureText(text_, font_size_);
  renderer.DrawText(text_, position_, font_size_, color_);
}

void Label::SetText(const std::string& text) { text_ = text; }

}  // namespace client::ui
