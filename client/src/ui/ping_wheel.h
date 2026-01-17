#ifndef CLIENT_UI_PING_WHEEL_H_
#define CLIENT_UI_PING_WHEEL_H_

#include <vector>
#include <string>
#include <optional>
#include <functional>

#include "engine/render/renderer2d.h"
#include "engine/input.h"
#include "protocol/gameplay_ping.h"

namespace client::ui {

struct PingOption {
  protocol::PingType type;
  std::string label;
  engine::render::Color color;
};

class PingWheel {
 public:
  PingWheel();

  void Update(engine::input::InputManager& input, const engine::math::Vector2i& window_size);
  void Draw(engine::render::Renderer2D& renderer);

  bool IsActive() const { return active_; }
  std::optional<protocol::PingType> GetSelection() const { return selection_; }
  
  // Call this when 'G' is released to finalize selection
  std::optional<protocol::PingType> CommitSelection();

 private:
  bool active_{false};
  std::optional<protocol::PingType> selection_;
  engine::math::Vector2f center_pos_;
  
  const std::vector<PingOption> options_;
  
  static constexpr float kWheelRadius = 150.0f;
  static constexpr float kInnerRadius = 50.0f;
};

} // namespace client::ui

#endif // CLIENT_UI_PING_WHEEL_H_
