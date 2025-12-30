#ifndef ENGINE_PROFILING_PROFILING_OVERLAY_H_
#define ENGINE_PROFILING_PROFILING_OVERLAY_H_

#include <optional>

#include "engine/math/vector2.h"
#include "engine/profiling/frame_profiler.h"
#include "engine/profiling/network_profiler.h"
#include "engine/profiling/resource_monitor.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

namespace engine::profiling {

struct OverlayConfig {
  float font_size{14.0f};
  float padding{8.0f};
  float line_spacing{4.0f};
  float graph_height{40.0f};
  float graph_width{200.0f};
  float corner_offset{10.0f};

  render::Color background_color{render::Color::FromBytes(0, 0, 0, 180)};
  render::Color text_color{render::Color::FromBytes(240, 240, 240)};

  render::Color graph_good{render::Color::FromBytes(80, 200, 80)};
  render::Color graph_warning{render::Color::FromBytes(220, 180, 50)};
  render::Color graph_critical{render::Color::FromBytes(220, 80, 80)};
  render::Color graph_background{render::Color::FromBytes(30, 30, 30)};

  float frame_time_warning_ms{20.0f};
  float frame_time_critical_ms{33.3f};
  float latency_warning_ms{80.0f};
  float latency_critical_ms{150.0f};

  bool show_frame_graph{true};
  bool show_latency_graph{true};
  bool show_resource_stats{true};
  bool show_world_position{true};
};

class ProfilingOverlay {
 public:
  ProfilingOverlay();
  explicit ProfilingOverlay(const OverlayConfig& config);

  void Toggle();
  void SetEnabled(bool enabled);
  bool enabled() const { return enabled_; }

  void Update(time::TimeDelta dt);
  void UpdateLatency(float latency_ms);
  void UpdatePacketReceived();
  void UpdatePacketDropped();
  void UpdateWorldPosition(const math::Vector2f& position);
  void UpdateEntityCount(std::size_t count);

  void Draw(render::Renderer2D& renderer,
            const math::Vector2i& window_size) const;

  OverlayConfig& config() { return config_; }
  const OverlayConfig& config() const { return config_; }

  FrameProfiler& frame_profiler() { return frame_profiler_; }
  NetworkProfiler& network_profiler() { return network_profiler_; }
  ResourceMonitor& resource_monitor() { return resource_monitor_; }

 private:
  template <std::size_t N>
  void DrawGraphImpl(render::Renderer2D& renderer, float x, float y,
                     float width, float height,
                     const RingBuffer<float, N>& samples,
                     std::size_t visible_bars, float max_value,
                     float warning_threshold, float critical_threshold) const;

  render::Color GetGraphColor(float value, float warning, float critical) const;

  OverlayConfig config_;
  FrameProfiler frame_profiler_;
  NetworkProfiler network_profiler_;
  ResourceMonitor resource_monitor_;

  std::optional<math::Vector2f> world_position_;
  std::size_t entity_count_{0};
  bool enabled_{false};
};

}  // namespace engine::profiling

#endif  // ENGINE_PROFILING_PROFILING_OVERLAY_H_
