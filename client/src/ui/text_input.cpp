#include "text_input.h"

#include <cmath>
#include <iostream>

#include "engine/render/renderer2d.h"
#include "engine/util/logging.h"

namespace client::ui {

namespace {

char KeyToChar(engine::input::Key key, bool shift) {
  using engine::input::Key;

  if (key >= Key::kA && key <= Key::kZ) {
    char base = shift ? 'A' : 'a';
    char c = base + (static_cast<int>(key) - static_cast<int>(Key::kA));

    // AZERTY Swaps
    if (key == Key::kA)
      c = shift ? 'Q' : 'q';
    else if (key == Key::kQ)
      c = shift ? 'A' : 'a';
    else if (key == Key::kZ)
      c = shift ? 'W' : 'w';
    else if (key == Key::kW)
      c = shift ? 'Z' : 'z';
    else if (key == Key::kM)
      return shift ? '?' : ',';

    return c;
  }

  if (key == Key::kSemicolon) return shift ? 'M' : 'm';
  if (key == Key::kComma) return shift ? '.' : ';';
  if (key == Key::kPeriod) return shift ? '/' : ':';
  if (key >= Key::kNum0 && key <= Key::kNum9) {
    char base = '0';
    return base + (static_cast<int>(key) - static_cast<int>(Key::kNum0));
  }

  if (key == Key::kSpace) return ' ';
  if (key == Key::kMinus) return shift ? '_' : '-';

  return 0;
}

}  // namespace

TextInput::TextInput(engine::math::Vector2f position,
                     engine::math::Vector2f size)
    : position_(position), size_(size) {
  last_key_states_.resize(100, false);
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
    if (!text_.empty()) {
      renderer.DrawText(display_text + "|", {position_.x + 10.0f, text_y},
                        font_size, color);
    } else {
      renderer.DrawText("|", {position_.x + 10.0f, text_y}, font_size,
                        text_color_);
    }
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
        engine::util::Logger::Default().Info(
            "TextInput: Backspace. Current text: ", text_);
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
    if (idx < 0 || static_cast<std::size_t>(idx) >= last_key_states_.size())
      return;

    bool down = input.IsKeyDown(key);
    if (down && !last_key_states_[idx]) {
      char c = KeyToChar(key, shift);
      if (c != 0) {
        text_ += c;
        engine::util::Logger::Default().Info("TextInput: Added char '", c,
                                             "'. Current text: ", text_);
      }
    }
    last_key_states_[idx] = down;
  };

  for (int k = static_cast<int>(Key::kA); k <= static_cast<int>(Key::kZ); ++k) {
    check_key(static_cast<Key>(k));
  }
  for (int k = static_cast<int>(Key::kNum0); k <= static_cast<int>(Key::kNum9);
       ++k) {
    check_key(static_cast<Key>(k));
  }
  check_key(Key::kSpace);
  check_key(Key::kPeriod);
  check_key(Key::kMinus);
  check_key(Key::kSemicolon);
  check_key(Key::kComma);
}

}  // namespace client::ui
