#include "engine/console/console_overlay.h"

#include <algorithm>
#include <sstream>

#include "engine/input/key_mappings.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"

namespace engine::console {

ConsoleOverlay::ConsoleOverlay() : ConsoleOverlay(Console::Instance()) {}

ConsoleOverlay::ConsoleOverlay(Console& console)
    : ConsoleOverlay(console, ConsoleOverlayConfig{}) {}

ConsoleOverlay::ConsoleOverlay(Console& console,
                               const ConsoleOverlayConfig& config)
    : console_(&console), config_(config) {
  key_states_.resize(static_cast<std::size_t>(input::Key::kF4) + 1, false);
}

void ConsoleOverlay::Toggle() {
  open_ = !open_;
  if (open_) {
    show_cursor_ = true;
    cursor_blink_timer_ = 0.0f;
    autocomplete_suggestions_.clear();
    show_autocomplete_ = false;
  }
}

void ConsoleOverlay::SetOpen(bool open) {
  if (open_ != open) {
    Toggle();
  }
}

void ConsoleOverlay::Update(time::TimeDelta dt, input::InputManager& input) {
  bool toggle_down = input.IsKeyDown(config_.toggle_key);
  if (toggle_down && !toggle_key_was_down_) {
    Toggle();
  }
  toggle_key_was_down_ = toggle_down;

  if (!open_) {
    return;
  }

  cursor_blink_timer_ += dt.as_seconds();
  if (cursor_blink_timer_ >= 0.5f) {
    show_cursor_ = !show_cursor_;
    cursor_blink_timer_ = 0.0f;
  }

  HandleInput(input);
}

void ConsoleOverlay::HandleInput(input::InputManager& input) {
  using Key = input::Key;

  auto check_key = [&](Key key) -> bool {
    int idx = static_cast<int>(key);
    if (idx < 0 || static_cast<std::size_t>(idx) >= key_states_.size()) {
      return false;
    }
    bool down = input.IsKeyDown(key);
    bool pressed = down && !key_states_[idx];
    key_states_[idx] = down;
    return pressed;
  };

  if (check_key(Key::kEnter)) {
    if (show_autocomplete_ && autocomplete_index_ >= 0) {
      ApplyAutocomplete();
    } else {
      Submit();
    }
    return;
  }

  if (check_key(Key::kEscape)) {
    if (show_autocomplete_) {
      show_autocomplete_ = false;
      autocomplete_suggestions_.clear();
    } else {
      Toggle();
    }
    return;
  }

  bool shift_held =
      input.IsKeyDown(Key::kLeftShift) || input.IsKeyDown(Key::kRightShift);

  if (check_key(Key::kUp)) {
    if (shift_held) {
      ScrollUp();
    } else if (show_autocomplete_ && !autocomplete_suggestions_.empty()) {
      autocomplete_index_ =
          (autocomplete_index_ <= 0)
              ? static_cast<int>(autocomplete_suggestions_.size()) - 1
              : autocomplete_index_ - 1;
    } else {
      HistoryUp();
    }
    return;
  }

  if (check_key(Key::kDown)) {
    if (shift_held) {
      ScrollDown();
    } else if (show_autocomplete_ && !autocomplete_suggestions_.empty()) {
      autocomplete_index_ =
          (autocomplete_index_ >=
           static_cast<int>(autocomplete_suggestions_.size()) - 1)
              ? 0
              : autocomplete_index_ + 1;
    } else {
      HistoryDown();
    }
    return;
  }

  bool ctrl_held =
      input.IsKeyDown(Key::kLeftControl) || input.IsKeyDown(Key::kRightControl);
  if (ctrl_held && check_key(Key::kSpace)) {
    UpdateAutocomplete();
    return;
  }

  HandleTextInput(input);
}

void ConsoleOverlay::HandleTextInput(input::InputManager& input) {
  using Key = input::Key;

  auto check_key = [&](Key key) -> bool {
    int idx = static_cast<int>(key);
    if (idx < 0 || static_cast<std::size_t>(idx) >= key_states_.size()) {
      return false;
    }
    bool down = input.IsKeyDown(key);
    bool pressed = down && !key_states_[idx];
    key_states_[idx] = down;
    return pressed;
  };

  if (check_key(Key::kBackspace)) {
    if (!input_text_.empty()) {
      input_text_.pop_back();
      show_autocomplete_ = false;
    }
    return;
  }

  bool shift =
      input.IsKeyDown(Key::kLeftShift) || input.IsKeyDown(Key::kRightShift);

  for (const auto& mapping : input::KeyMappings()) {
    if (check_key(mapping.key)) {
      auto text = input::KeyText(mapping.key, shift);
      if (!text.empty()) {
        input_text_ += text;
        show_autocomplete_ = false;
      }
    }
  }
}

void ConsoleOverlay::Submit() {
  if (input_text_.empty()) {
    return;
  }

  if (console_) {
    console_->Execute(input_text_);
  }

  input_text_.clear();
  history_index_ = -1;
  scroll_offset_ = 0;
  show_autocomplete_ = false;
  autocomplete_suggestions_.clear();
}

void ConsoleOverlay::UpdateAutocomplete() {
  if (!console_ || input_text_.empty()) {
    show_autocomplete_ = false;
    autocomplete_suggestions_.clear();
    return;
  }

  autocomplete_suggestions_ = console_->Autocomplete(input_text_);
  show_autocomplete_ = !autocomplete_suggestions_.empty();
  autocomplete_index_ = show_autocomplete_ ? 0 : -1;
}

void ConsoleOverlay::ApplyAutocomplete() {
  if (autocomplete_index_ >= 0 &&
      autocomplete_index_ <
          static_cast<int>(autocomplete_suggestions_.size())) {
    input_text_ = autocomplete_suggestions_[autocomplete_index_];
  }
  show_autocomplete_ = false;
  autocomplete_suggestions_.clear();
  autocomplete_index_ = -1;
}

void ConsoleOverlay::HistoryUp() {
  if (!console_) return;

  const auto& history = console_->GetInputHistory();
  if (history.empty()) return;

  if (history_index_ < 0) {
    history_index_ = static_cast<int>(history.size()) - 1;
  } else if (history_index_ > 0) {
    --history_index_;
  }

  if (history_index_ >= 0 &&
      history_index_ < static_cast<int>(history.size())) {
    input_text_ = history[history_index_];
  }
}

void ConsoleOverlay::HistoryDown() {
  if (!console_) return;

  const auto& history = console_->GetInputHistory();
  if (history.empty() || history_index_ < 0) return;

  ++history_index_;

  if (history_index_ >= static_cast<int>(history.size())) {
    history_index_ = -1;
    input_text_.clear();
  } else {
    input_text_ = history[history_index_];
  }
}

void ConsoleOverlay::ScrollUp() {
  constexpr std::size_t kScrollStep = 3;
  if (console_) {
    std::size_t max_scroll = console_->GetHistorySize();
    scroll_offset_ = std::min(scroll_offset_ + kScrollStep, max_scroll);
  }
}

void ConsoleOverlay::ScrollDown() {
  constexpr std::size_t kScrollStep = 3;
  if (scroll_offset_ >= kScrollStep) {
    scroll_offset_ -= kScrollStep;
  } else {
    scroll_offset_ = 0;
  }
}

render::Color ConsoleOverlay::GetLineColor(ConsoleLine::Type type) const {
  switch (type) {
    case ConsoleLine::Type::kError:
      return config_.error_color;
    case ConsoleLine::Type::kInfo:
      return config_.info_color;
    case ConsoleLine::Type::kInput:
      return config_.prompt_color;
    default:
      return config_.text_color;
  }
}

void ConsoleOverlay::Draw(render::Renderer2D& renderer,
                          const math::Vector2i& window_size) {
  if (!open_) {
    return;
  }

  float console_height =
      static_cast<float>(window_size.y) * config_.height_ratio;
  float console_width = static_cast<float>(window_size.x);

  math::RectF bg_rect{0.0f, 0.0f, console_width, console_height};
  renderer.DrawRect(bg_rect, config_.background_color);

  float input_y = console_height - config_.input_height - config_.padding;

  renderer.DrawLine({0.0f, input_y - 2.0f}, {console_width, input_y - 2.0f},
                    1.0f, config_.text_color);

  math::RectF input_rect{config_.padding, input_y,
                         console_width - config_.padding * 2.0f,
                         config_.input_height};
  renderer.DrawRect(input_rect, config_.input_background);

  float text_x = config_.padding + 4.0f;
  float input_text_y =
      input_y + (config_.input_height - config_.font_size) * 0.5f;

  renderer.DrawText(">", {text_x, input_text_y}, config_.font_size,
                    config_.prompt_color);

  float prompt_width = renderer.MeasureText(">", config_.font_size).x;
  float input_start_x = text_x + prompt_width + 4.0f;

  std::string visible_input = input_text_;
  float input_width = renderer.MeasureText(visible_input, config_.font_size).x;
  float max_input_width =
      console_width - input_start_x - config_.padding - 20.0f;

  while (input_width > max_input_width && !visible_input.empty()) {
    visible_input = visible_input.substr(1);
    input_width = renderer.MeasureText(visible_input, config_.font_size).x;
  }

  renderer.DrawText(visible_input, {input_start_x, input_text_y},
                    config_.font_size, config_.input_color);

  if (show_cursor_) {
    float cursor_x = input_start_x + input_width;
    renderer.DrawText("|", {cursor_x, input_text_y}, config_.font_size,
                      config_.cursor_color);
  }

  float output_top = config_.padding;
  float output_bottom = input_y - config_.padding - 4.0f;
  float output_height = output_bottom - output_top;
  float line_height = config_.font_size + config_.line_spacing;

  if (console_ && output_height > 0) {
    const auto& history = console_->GetHistory();

    std::vector<std::pair<std::string, ConsoleLine::Type>> display_lines;
    for (const auto& entry : history) {
      std::istringstream stream(entry.text);
      std::string line;
      while (std::getline(stream, line)) {
        if (line.length() > 120) {
          line = line.substr(0, 117) + "...";
        }
        display_lines.emplace_back(line, entry.type);
      }
    }

    int visible_count = static_cast<int>(output_height / line_height);
    int total_lines = static_cast<int>(display_lines.size());

    int end_line = std::max(0, total_lines - static_cast<int>(scroll_offset_));
    int start_line = std::max(0, end_line - visible_count);

    float y = output_top;
    for (int i = start_line; i < end_line && y + line_height <= output_bottom;
         ++i) {
      const auto& [text, type] = display_lines[i];
      render::Color color = GetLineColor(type);
      renderer.DrawText(text, {text_x, y}, config_.font_size, color);
      y += line_height;
    }
  }

  if (show_autocomplete_ && !autocomplete_suggestions_.empty()) {
    float ac_x = input_start_x;
    float ac_line_height = config_.font_size + 4.0f;
    std::size_t max_suggestions = 8;
    std::size_t visible_suggestions =
        std::min(autocomplete_suggestions_.size(), max_suggestions);
    float ac_height =
        static_cast<float>(visible_suggestions) * ac_line_height + 8.0f;
    float ac_width = 250.0f;

    float ac_y = input_y - config_.padding - ac_height;
    if (ac_y < output_top) {
      ac_y = output_top;
    }

    math::RectF ac_bg{ac_x, ac_y, ac_width, ac_height};
    renderer.DrawRect(ac_bg, config_.autocomplete_bg);

    float item_y = ac_y + 4.0f;
    for (std::size_t i = 0; i < visible_suggestions; ++i) {
      if (static_cast<int>(i) == autocomplete_index_) {
        math::RectF highlight{ac_x, item_y - 2.0f, ac_width, ac_line_height};
        renderer.DrawRect(highlight, config_.autocomplete_highlight);
      }

      std::string suggestion = autocomplete_suggestions_[i];
      if (suggestion.length() > 30) {
        suggestion = suggestion.substr(0, 27) + "...";
      }

      renderer.DrawText(suggestion, {ac_x + 4.0f, item_y}, config_.font_size,
                        config_.text_color);
      item_y += ac_line_height;
    }
  }
}

}  // namespace engine::console
