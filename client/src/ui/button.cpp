#include "button.h"

namespace client::ui {

Button::Button(engine::math::Vector2f position, engine::math::Vector2f size,
               const std::string& text, std::function<void()> on_click)
    : position_(position), size_(size), text_(text), on_click_(on_click) {}

void Button::Update(engine::time::TimeDelta /*dt*/,
                    engine::input::InputManager& input) {
  auto mouse_pos = input.GetMousePosition();
  engine::math::RectF rect{position_.x, position_.y, size_.x, size_.y};

  hovered_ = rect.Contains(mouse_pos);

  bool left_down = input.IsMouseButtonDown(engine::input::MouseButton::kLeft);

  if (hovered_) {
    if (left_down) {
      pressed_ = true;
    } else if (pressed_) {
      pressed_ = false;
      if (on_click_) on_click_();
    }
  } else {
    pressed_ = false;
  }
}

void Button::Draw(engine::render::Renderer2D& renderer) {
  engine::render::Color color = normal_color_;
  if (pressed_)
    color = pressed_color_;
  else if (hovered_)
    color = hover_color_;

  if (texture_) {
    engine::render::SpriteDrawParams params;
    params.position = position_;
    params.scale = {size_.x / static_cast<float>(texture_->GetSize().x),
                    size_.y / static_cast<float>(texture_->GetSize().y)};
    params.tint = color;
    renderer.DrawTexture(*texture_, params);
  } else {
    renderer.DrawRect({position_.x, position_.y, size_.x, size_.y}, color);
  }

  if (!text_.empty()) {
    float font_size = size_.y * text_scale_;
    auto text_size = renderer.MeasureText(text_, font_size);
    float text_x = position_.x + (size_.x - text_size.x) * 0.5f;
    float text_y = position_.y + (size_.y - text_size.y) * 0.5f;

    renderer.DrawText(text_, {text_x, text_y}, font_size, text_color_);
  }
}

void Button::SetTexture(std::shared_ptr<engine::render::Texture2D> texture) {
  texture_ = texture;
}

void Button::SetColors(engine::render::Color normal,
                       engine::render::Color hover,
                       engine::render::Color pressed) {
  normal_color_ = normal;
  hover_color_ = hover;
  pressed_color_ = pressed;
}

}  // namespace client::ui
