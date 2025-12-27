#include "engine/ui/text_input.h"

#include <array>
#include <string_view>

#include "engine/render/renderer2d.h"

namespace engine::ui {

namespace {

struct KeyCharacterMapping {
  engine::input::Key key;
  std::string_view normal;
  std::string_view shifted;
};

// Keep in sync with the physical AZERTY scancode mapping in the raylib backend.
constexpr std::array<KeyCharacterMapping, 44> kCharacterMappings{{
    {engine::input::Key::kA, "a", "A"},
    {engine::input::Key::kB, "b", "B"},
    {engine::input::Key::kC, "c", "C"},
    {engine::input::Key::kD, "d", "D"},
    {engine::input::Key::kE, "e", "E"},
    {engine::input::Key::kF, "f", "F"},
    {engine::input::Key::kG, "g", "G"},
    {engine::input::Key::kH, "h", "H"},
    {engine::input::Key::kI, "i", "I"},
    {engine::input::Key::kJ, "j", "J"},
    {engine::input::Key::kK, "k", "K"},
    {engine::input::Key::kL, "l", "L"},
    {engine::input::Key::kM, "m", "M"},
    {engine::input::Key::kN, "n", "N"},
    {engine::input::Key::kO, "o", "O"},
    {engine::input::Key::kP, "p", "P"},
    {engine::input::Key::kQ, "q", "Q"},
    {engine::input::Key::kR, "r", "R"},
    {engine::input::Key::kS, "s", "S"},
    {engine::input::Key::kT, "t", "T"},
    {engine::input::Key::kU, "u", "U"},
    {engine::input::Key::kV, "v", "V"},
    {engine::input::Key::kW, "w", "W"},
    {engine::input::Key::kX, "x", "X"},
    {engine::input::Key::kY, "y", "Y"},
    {engine::input::Key::kZ, "z", "Z"},
    {engine::input::Key::kNum0, "\u00e0", "0"},
    {engine::input::Key::kNum1, "&", "1"},
    {engine::input::Key::kNum2, "\u00e9", "2"},
    {engine::input::Key::kNum3, "\"", "3"},
    {engine::input::Key::kNum4, "'", "4"},
    {engine::input::Key::kNum5, "(", "5"},
    {engine::input::Key::kNum6, "-", "6"},
    {engine::input::Key::kNum7, "\u00e8", "7"},
    {engine::input::Key::kNum8, "_", "8"},
    {engine::input::Key::kNum9, "\u00e7", "9"},
    {engine::input::Key::kMinus, ")", "\u00b0"},
    {engine::input::Key::kEqual, "=", "+"},
    {engine::input::Key::kSpace, " ", " "},
    {engine::input::Key::kComma, ",", "?"},
    {engine::input::Key::kSemicolon, ";", "."},
    {engine::input::Key::kPeriod, ":", "/"},
    {engine::input::Key::kSlash, "!", "\u00a7"},
    {engine::input::Key::kBackslash, "*", "\u00b5"},
}};

std::string KeyToText(engine::input::Key key, bool shift) {
  for (const auto& mapping : kCharacterMappings) {
    if (mapping.key == key) {
      return shift ? std::string(mapping.shifted) : std::string(mapping.normal);
    }
  }
  return {};
}

}  // namespace

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
      std::string text = KeyToText(key, shift);
      if (!text.empty()) {
        text_ += text;
      }
    }
    last_key_states_[idx] = down;
  };

  for (const auto& mapping : kCharacterMappings) {
    check_key(mapping.key);
  }
}

}  // namespace engine::ui
