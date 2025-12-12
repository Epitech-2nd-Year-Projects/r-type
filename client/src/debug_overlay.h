/**
 * @file debug_overlay.h
 * @brief Toggleable on-screen developer HUD with timing and latency metrics
 */

#ifndef CLIENT_DEBUG_OVERLAY_H_
#define CLIENT_DEBUG_OVERLAY_H_

#include <array>
#include <cstddef>
#include <optional>

#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

class DebugOverlayTestPeer;

namespace client {

/**
 * @class DebugOverlay
 * @brief Aggregates frame and network metrics for a lightweight debug HUD
 *
 * @details
 * Tracks frame timing history, latest latency measurement, and renderable
 * entity counts. Call the update methods once per frame and draw after the
 * main scene to render the overlay when enabled.
 */
class DebugOverlay {
 public:
  DebugOverlay() = default;

  /**
   * @brief Toggle overlay visibility.
   */
  void Toggle();

  /**
   * @brief Overlay visibility state.
   */
  bool enabled() const { return enabled_; }

  /**
   * @brief Record the latest frame duration for FPS and average timing.
   */
  void UpdateFrameTiming(engine::time::TimeDelta dt);

  /**
   * @brief Update the most recent measured latency.
   * @param latency_ms Round-trip time in milliseconds.
   */
  void UpdateLatency(std::optional<float> latency_ms);

  /**
   * @brief Update the count of renderable entities.
   */
  void UpdateRenderableCount(std::size_t entities);

  /**
   * @brief Draw the overlay when enabled.
   * @param renderer Renderer used for drawing text and backgrounds.
   * @param window_size Current window dimensions for positioning.
   */
 void Draw(engine::render::Renderer2D& renderer,
           const engine::math::Vector2i& window_size) const;

 private:
  friend class ::DebugOverlayTestPeer;
  float AverageFrameTimeMs() const;

  /**
   * @brief Number of recent frames to track for timing metrics.
   *
   * Represents a rolling window of 120 frames (about two seconds at 60 FPS).
   * Adjust to change the duration of timing history shown in the overlay.
   */
  static constexpr std::size_t kSampleCount = 120;
  std::array<float, kSampleCount> frame_times_ms_{};
  std::size_t samples_recorded_{0};
  std::size_t next_sample_index_{0};
  float latest_fps_{0.0f};
  std::optional<float> last_latency_ms_{};
  std::size_t last_entity_count_{0};
  bool enabled_{false};
};

}  // namespace client

#endif  // CLIENT_DEBUG_OVERLAY_H_
