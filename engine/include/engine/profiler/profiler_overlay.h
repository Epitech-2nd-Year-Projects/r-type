#ifndef ENGINE_PROFILER_PROFILER_OVERLAY_H_
#define ENGINE_PROFILER_PROFILER_OVERLAY_H_

#include "engine/profiler/profiler.h"
#include "engine/render/renderer2d.h"

namespace engine::profiler {

class ProfilerOverlay {
 public:
  ProfilerOverlay() = default;

  void Draw(render::Renderer2D& renderer, const math::Vector2i& window_size);
  void Toggle();
  void SetEnabled(bool enabled);
  bool IsEnabled() const;

 private:
  void DrawGraph(render::Renderer2D& renderer, const std::string& name,
                 const MetricHistory& history, float x, float& y, float width);

  bool enabled_{false};
};

}  // namespace engine::profiler

#endif  // ENGINE_PROFILER_PROFILER_OVERLAY_H_
