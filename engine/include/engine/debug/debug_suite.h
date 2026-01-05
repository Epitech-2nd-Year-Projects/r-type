#ifndef ENGINE_DEBUG_DEBUG_SUITE_H_
#define ENGINE_DEBUG_DEBUG_SUITE_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "engine/debug/component_inspector_registry.h"
#include "engine/debug/entity_hierarchy_panel.h"
#include "engine/debug/imgui_integration.h"
#include "engine/debug/inspector_panel.h"
#include "engine/ecs/registry.h"

namespace engine::debug {
class NetworkDebugger;
}

namespace engine::console {
class ConsoleOverlay;
}

namespace engine::profiling {
class ProfilingOverlay;
}

namespace engine::debug {

class DebugSuite {
 public:
  struct Config {
    bool show_hierarchy{true};
    bool show_inspector{true};
    bool show_network{false};
    bool show_profiler{true};
    // Console visibility is tracked via the ConsoleOverlay itself usually
    bool show_demo_window{false};
  };

  DebugSuite(ecs::Registry& registry,
             const ComponentInspectorRegistry& inspector_registry);

  void SetNetworkDebugger(
      std::reference_wrapper<NetworkDebugger> network_debugger);
  void SetConsoleOverlay(
      std::reference_wrapper<engine::console::ConsoleOverlay> console_overlay);
  void SetProfilingOverlay(
      std::reference_wrapper<engine::profiling::ProfilingOverlay>
          profiling_overlay);

  void RegisterGizmo(std::string name, std::reference_wrapper<bool> value);

  bool IsProfilerEnabled() const { return config_.show_profiler; }

  void Draw();

 private:
  void DrawMainMenuBar();
  void DrawDockingWindow();

  Config config_;
  EntityHierarchyPanel hierarchy_panel_;
  InspectorPanel inspector_panel_;

  std::optional<std::reference_wrapper<NetworkDebugger>> network_debugger_;
  std::optional<std::reference_wrapper<engine::console::ConsoleOverlay>>
      console_overlay_;
  std::optional<std::reference_wrapper<engine::profiling::ProfilingOverlay>>
      profiling_overlay_;

  std::vector<std::pair<std::string, std::reference_wrapper<bool>>> gizmos_;
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_DEBUG_SUITE_H_
