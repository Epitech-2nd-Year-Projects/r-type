#include "engine/profiling/profiling_overlay.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "engine/math/rect.h"

namespace engine::profiling {

ProfilingOverlay::ProfilingOverlay() = default;

ProfilingOverlay::ProfilingOverlay(const OverlayConfig& config)
    : config_(config) {}

void ProfilingOverlay::Toggle() { enabled_ = !enabled_; }

void ProfilingOverlay::SetEnabled(bool enabled) { enabled_ = enabled; }

void ProfilingOverlay::Update(time::TimeDelta dt) {
  frame_profiler_.RecordFrame(dt);
  resource_monitor_.Update();
}

void ProfilingOverlay::UpdateLatency(float latency_ms) {
  network_profiler_.RecordLatency(latency_ms);
}

void ProfilingOverlay::UpdatePacketReceived() {
  network_profiler_.RecordPacketReceived();
}

void ProfilingOverlay::UpdatePacketDropped() {
  network_profiler_.RecordPacketDropped();
}

void ProfilingOverlay::UpdateWorldPosition(const math::Vector2f& position) {
  world_position_ = position;
}

void ProfilingOverlay::UpdateEntityCount(std::size_t count) {
  entity_count_ = count;
}

void ProfilingOverlay::Draw(render::Renderer2D& renderer,
                            const math::Vector2i& window_size) const {
  if (!enabled_) return;

  auto frame_stats = frame_profiler_.GetStats();
  auto net_stats = network_profiler_.GetStats();
  auto res_stats = resource_monitor_.GetStats();

  std::vector<std::string> lines;
  {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1)
       << "FPS: " << frame_stats.current_fps
       << "  Frame: " << frame_stats.frame_time_ms << "ms";
    lines.push_back(ss.str());
  }
  if (config_.show_resource_stats) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1)
       << "CPU: " << res_stats.cpu_usage_percent
       << "%  Mem: " << res_stats.memory_usage_mb << " MB";
    lines.push_back(ss.str());
  }
  {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << "Ping: " << net_stats.latency_ms
       << "ms  Drop: " << net_stats.packets_dropped;
    lines.push_back(ss.str());
  }
  if (config_.show_world_position && world_position_.has_value()) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << "Pos: (" << world_position_->x
       << ", " << world_position_->y << ")";
    lines.push_back(ss.str());
  }
  {
    std::ostringstream ss;
    ss << "Entities: " << entity_count_;
    lines.push_back(ss.str());
  }

  float max_text_width = 0.0f;
  for (const auto& line : lines) {
    float w = renderer.MeasureText(line, config_.font_size).x;
    if (w > max_text_width) max_text_width = w;
  }

  float content_width = std::max(max_text_width, config_.graph_width);
  float box_width = content_width + config_.padding * 2.0f;

  float line_height = config_.font_size + config_.line_spacing;
  float text_height = static_cast<float>(lines.size()) * line_height;

  float graphs_height = 0.0f;
  if (config_.show_frame_graph) {
    graphs_height += config_.graph_height + config_.font_size + 4.0f;
  }
  if (config_.show_latency_graph) {
    graphs_height += config_.graph_height + config_.font_size + 4.0f;
  }

  float box_height = text_height + graphs_height + config_.padding * 2.0f;

  float origin_x =
      static_cast<float>(window_size.x) - box_width - config_.corner_offset;
  float origin_y = config_.corner_offset;

  math::RectF background{origin_x, origin_y, box_width, box_height};
  renderer.DrawRect(background, config_.background_color);

  float x = origin_x + config_.padding;
  float y = origin_y + config_.padding;

  for (const auto& line : lines) {
    renderer.DrawText(line, {x, y}, config_.font_size, config_.text_color);
    y += line_height;
  }

  if (config_.show_frame_graph) {
    renderer.DrawText("Frame Time", {x, y}, config_.font_size * 0.85f,
                      config_.text_color);
    y += config_.font_size;

    DrawGraphImpl(renderer, x, y, content_width, config_.graph_height,
                  frame_profiler_.frame_times(), 128, 50.0f,
                  config_.frame_time_warning_ms,
                  config_.frame_time_critical_ms);
    y += config_.graph_height + 4.0f;
  }

  if (config_.show_latency_graph) {
    renderer.DrawText("Latency", {x, y}, config_.font_size * 0.85f,
                      config_.text_color);
    y += config_.font_size;

    DrawGraphImpl(renderer, x, y, content_width, config_.graph_height,
                  network_profiler_.latency_samples(), 64, 200.0f,
                  config_.latency_warning_ms, config_.latency_critical_ms);
  }
}

template <std::size_t N>
void ProfilingOverlay::DrawGraphImpl(render::Renderer2D& renderer, float x,
                                     float y, float width, float height,
                                     const RingBuffer<float, N>& samples,
                                     std::size_t visible_bars, float max_value,
                                     float warning_threshold,
                                     float critical_threshold) const {
  math::RectF bg{x, y, width, height};
  renderer.DrawRect(bg, config_.graph_background);

  std::size_t count = samples.size();
  if (count == 0) return;

  std::size_t start_idx = (count > visible_bars) ? count - visible_bars : 0;
  std::size_t visible_count = count - start_idx;

  float bar_width = width / static_cast<float>(visible_bars);
  float gap = 1.0f;

  for (std::size_t i = 0; i < visible_count; ++i) {
    float value = samples[start_idx + i];
    float normalized = std::min(value / max_value, 1.0f);
    float bar_height = std::max(normalized * height, 1.0f);

    float bar_x = x + static_cast<float>(i) * bar_width;
    float bar_y = y + height - bar_height;

    render::Color color =
        GetGraphColor(value, warning_threshold, critical_threshold);
    float actual_bar_width = std::max(bar_width - gap, 1.0f);
    math::RectF bar{bar_x, bar_y, actual_bar_width, bar_height};
    renderer.DrawRect(bar, color);
  }
}

template void ProfilingOverlay::DrawGraphImpl<256>(
    render::Renderer2D&, float, float, float, float,
    const RingBuffer<float, 256>&, std::size_t, float, float, float) const;
template void ProfilingOverlay::DrawGraphImpl<128>(
    render::Renderer2D&, float, float, float, float,
    const RingBuffer<float, 128>&, std::size_t, float, float, float) const;

render::Color ProfilingOverlay::GetGraphColor(float value, float warning,
                                              float critical) const {
  if (value >= critical) return config_.graph_critical;
  if (value >= warning) return config_.graph_warning;
  return config_.graph_good;
}

}  // namespace engine::profiling
