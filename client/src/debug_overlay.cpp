#include "debug_overlay.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "constants/ui_constants.h"
#include "engine/math/rect.h"

namespace client {

void DebugOverlay::Toggle() { enabled_ = !enabled_; }

void DebugOverlay::UpdateFrameTiming(engine::time::TimeDelta dt) {
  const float seconds = dt.as_seconds();
  latest_fps_ = seconds > 0.0f ? 1.0f / seconds : 0.0f;
  const float frame_ms = seconds * 1000.0f;

  frame_times_ms_[next_sample_index_] = frame_ms;
  next_sample_index_ = (next_sample_index_ + 1) % kSampleCount;
  samples_recorded_ =
      std::min(samples_recorded_ + 1, static_cast<std::size_t>(kSampleCount));
}

void DebugOverlay::UpdateLatency(std::optional<float> latency_ms) {
  last_latency_ms_ = latency_ms;
}

void DebugOverlay::UpdateRenderableCount(std::size_t entities) {
  last_entity_count_ = entities;
}

float DebugOverlay::AverageFrameTimeMs() const {
  if (samples_recorded_ == 0) {
    return 0.0f;
  }
  float sum = 0.0f;
  const std::size_t limit = std::min(samples_recorded_, kSampleCount);
  for (std::size_t i = 0; i < limit; ++i) {
    sum += frame_times_ms_[i];
  }
  return sum / static_cast<float>(limit);
}

void DebugOverlay::Draw(engine::render::Renderer2D& renderer,
                        const engine::math::Vector2i& window_size) const {
  if (!enabled_) {
    return;
  }

  std::ostringstream fps_stream;
  fps_stream << std::fixed << std::setprecision(1) << latest_fps_;

  std::ostringstream frame_stream;
  frame_stream << std::fixed << std::setprecision(2) << AverageFrameTimeMs();

  std::string latency_line = "Latency: --";
  if (last_latency_ms_.has_value()) {
    std::ostringstream latency_stream;
    latency_stream << std::fixed << std::setprecision(1) << *last_latency_ms_
                   << " ms";
    latency_line = "Latency: " + latency_stream.str();
  }

  std::ostringstream entities_stream;
  entities_stream << last_entity_count_;

  std::vector<std::string> lines;
  lines.reserve(4);
  lines.push_back("FPS: " + fps_stream.str());
  lines.push_back("Frame avg: " + frame_stream.str() + " ms");
  lines.push_back(latency_line);
  lines.push_back("Entities: " + entities_stream.str());

  float max_width = 0.0f;
  for (const auto& line : lines) {
    const float width =
        renderer.MeasureText(line, constants::ui::DebugOverlay::kFontSize).x;
    max_width = std::max(max_width, width);
  }

  const float line_height = constants::ui::DebugOverlay::kFontSize +
                            constants::ui::DebugOverlay::kLineSpacing;
  const float box_width =
      max_width + constants::ui::DebugOverlay::kPadding * 2.0f;
  const float box_height = static_cast<float>(lines.size()) * line_height +
                           constants::ui::DebugOverlay::kPadding * 2.0f;
  const float origin_x =
      std::max(constants::ui::DebugOverlay::kCornerOffset,
               static_cast<float>(window_size.x) - box_width -
                   constants::ui::DebugOverlay::kCornerOffset);
  const float origin_y = constants::ui::DebugOverlay::kCornerOffset;
  const engine::math::RectF background{origin_x, origin_y, box_width,
                                       box_height};
  renderer.DrawRect(background, constants::ui::DebugOverlay::kBackgroundColor);

  float y = background.top_left_y_ + constants::ui::DebugOverlay::kPadding;
  const float x =
      background.top_left_x_ + constants::ui::DebugOverlay::kPadding;
  for (const auto& line : lines) {
    renderer.DrawText(line, {x, y}, constants::ui::DebugOverlay::kFontSize,
                      constants::ui::DebugOverlay::kTextColor);
    y += line_height;
  }
}

}  // namespace client
