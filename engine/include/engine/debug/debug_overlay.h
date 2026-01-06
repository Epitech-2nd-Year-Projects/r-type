#ifndef ENGINE_DEBUG_DEBUG_OVERLAY_H_
#define ENGINE_DEBUG_DEBUG_OVERLAY_H_

#include <string>
#include <utility>
#include <vector>

#include "engine/debug/entity_hierarchy_panel.h"
#include "engine/debug/inspector_panel.h"
#include "engine/ecs/registry.h"

namespace engine::debug {

class ComponentInspectorRegistry;

/**
 * @class DebugOverlay
 * @brief Unified container for all debug panels
 */
class DebugOverlay {
 public:
  DebugOverlay(engine::ecs::Registry& registry,
               const ComponentInspectorRegistry& inspector_registry);

  void Draw();

  void RegisterDebugToggle(std::string label, bool* value);

 private:
  EntityHierarchyPanel hierarchy_panel_;
  InspectorPanel inspector_panel_;
  std::vector<std::pair<std::string, bool*>> toggles_;
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_DEBUG_OVERLAY_H_
