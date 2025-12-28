#include "engine/ui/text_input.h"

#include "engine/input/key_mappings.h"
#include "engine/render/renderer2d.h"

namespace engine::ui {

TextInput::TextInput(engine::math::Vector2f position,
                     engine::math::Vector2f size)
    : position_(position), size_(size) {
  last_key_states_.resize(
      static_cast<std::size_t>(engine::input::Key::kRightAlt) + 1, false);
}

void TextInput::Update(engine::time::TimeDelta dt,
                       engine::input::InputManager& input) {
  if (focused_) {
    blink_timer_ += dt.as_seconds();
    if (blink_timer_ >= 0.5f) {
      show_cursor_ = !show_cursor_;
      blink_timer_ = 0.0f;
    }
    HandleTyping(input);
  } else {
    show_cursor_ = false;
  }
}

void TextInput::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawRect({position_.x, position_.y, size_.x, size_.y},
                    background_color_);

  engine::render::Color border =
      focused_ ? focused_border_color_ : border_color_;
  renderer.DrawLine(position_, {position_.x + size_.x, position_.y}, 2.0f,
                    border);
  renderer.DrawLine({position_.x + size_.x, position_.y},
                    {position_.x + size_.x, position_.y + size_.y}, 2.0f,
                    border);
  renderer.DrawLine({position_.x + size_.x, position_.y + size_.y},
                    {position_.x, position_.y + size_.y}, 2.0f, border);
  renderer.DrawLine({position_.x, position_.y + size_.y}, position_, 2.0f,
                    border);

  std::string display_text = text_;
  engine::render::Color color = text_color_;

  if (text_.empty() && !placeholder_.empty()) {
    display_text = placeholder_;
    color = engine::render::Color::FromBytes(128, 128, 128);
  }

  float font_size = size_.y * 0.6f;
  float text_y = position_.y + (size_.y - font_size) * 0.5f;
  renderer.DrawText(display_text, {position_.x + 10.0f, text_y}, font_size,
                    color);

  if (focused_ && show_cursor_) {
    float cursor_x = position_.x + 10.0f;
    if (!text_.empty()) {
      cursor_x += renderer.MeasureText(text_, font_size).x;
    }
    renderer.DrawText("|", {cursor_x, text_y}, font_size, text_color_);
  }
}

void TextInput::SetText(const std::string& text) { text_ = text; }

const std::string& TextInput::GetText() const { return text_; }

void TextInput::SetPlaceholder(const std::string& placeholder) {
  placeholder_ = placeholder;
}

void TextInput::SetFocused(bool focused) {
  focused_ = focused;
  if (focused) {
    blink_timer_ = 0.0f;
    show_cursor_ = true;
  }
}

void TextInput::HandleTyping(engine::input::InputManager& input) {
  using engine::input::Key;

  if (input.IsKeyDown(Key::kBackspace)) {
    int key_idx = static_cast<int>(Key::kBackspace);
    if (!last_key_states_[key_idx]) {
      if (!text_.empty()) {
        text_.pop_back();
      }
    }
    last_key_states_[key_idx] = true;
  } else {
    last_key_states_[static_cast<int>(Key::kBackspace)] = false;
  }

  bool shift =
      input.IsKeyDown(Key::kLeftShift) || input.IsKeyDown(Key::kRightShift);

  auto check_key = [&](Key key) {
    int idx = static_cast<int>(key);
    if (idx < 0 || static_cast<std::size_t>(idx) >= last_key_states_.size()) {
      return;
    }

    bool down = input.IsKeyDown(key);
    if (down && !last_key_states_[idx]) {
      const auto text = engine::input::KeyText(key, shift);
      if (!text.empty()) {
        text_ += text;
      }
    }
    last_key_states_[idx] = down;
  };

  for (const auto& mapping : engine::input::KeyMappings()) {
    check_key(mapping.key);
  }
}

}  // namespace engine::ui
