#ifndef ENGINE_CONSOLE_CONSOLE_OVERLAY_H_
#define ENGINE_CONSOLE_CONSOLE_OVERLAY_H_

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "engine/console/console.h"
#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/time/time_delta.h"

namespace engine::render {
class Renderer2D;
}

namespace engine::console {

struct ConsoleOverlayConfig {
  input::Key toggle_key{input::Key::kF1};

  float height_ratio{0.4f};
  float font_size{20.0f};
  float padding{10.0f};
  float line_spacing{6.0f};
  float input_height{36.0f};

  render::Color background_color{render::Color::FromBytes(20, 20, 25, 230)};
  render::Color input_background{render::Color::FromBytes(30, 30, 35, 255)};
  render::Color text_color{render::Color::FromBytes(220, 220, 220)};
  render::Color input_color{render::Color::FromBytes(255, 255, 255)};
  render::Color error_color{render::Color::FromBytes(255, 100, 100)};
  render::Color info_color{render::Color::FromBytes(150, 200, 255)};
  render::Color prompt_color{render::Color::FromBytes(100, 255, 100)};
  render::Color cursor_color{render::Color::FromBytes(255, 255, 255)};
  render::Color autocomplete_bg{render::Color::FromBytes(40, 40, 50, 240)};
  render::Color autocomplete_highlight{render::Color::FromBytes(60, 80, 120)};
};

class ConsoleOverlay {
 public:
  ConsoleOverlay();
  explicit ConsoleOverlay(Console& console);
  ConsoleOverlay(Console& console, const ConsoleOverlayConfig& config);

  void SetConsole(Console& console) { console_ = console; }

  void Toggle();
  void SetOpen(bool open);
  bool IsOpen() const { return open_; }

  void Update(time::TimeDelta dt, input::InputManager& input);
  void Draw(render::Renderer2D& renderer, const math::Vector2i& window_size);

  ConsoleOverlayConfig& config() { return config_; }
  const ConsoleOverlayConfig& config() const { return config_; }

 private:
  void HandleInput(input::InputManager& input);
  void HandleTextInput(input::InputManager& input);
  void Submit();
  void UpdateAutocomplete();
  void ApplyAutocomplete();

  void ScrollUp();
  void ScrollDown();
  void HistoryUp();
  void HistoryDown();

  render::Color GetLineColor(ConsoleLine::Type type) const;

  std::optional<std::reference_wrapper<Console>> console_;
  ConsoleOverlayConfig config_;

  bool open_{false};
  std::string input_text_;
  float cursor_blink_timer_{0.0f};
  bool show_cursor_{true};

  std::size_t scroll_offset_{0};
  int history_index_{-1};

  std::vector<std::string> autocomplete_suggestions_;
  int autocomplete_index_{-1};
  bool show_autocomplete_{false};

  std::vector<bool> key_states_;
  bool toggle_key_was_down_{false};
};

}  // namespace engine::console

#endif  // ENGINE_CONSOLE_CONSOLE_OVERLAY_H_
