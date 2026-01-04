/**
 * @file inspector_panel.h
 * @brief Panel for inspecting component data of selected entities
 */

#ifndef ENGINE_DEBUG_INSPECTOR_PANEL_H_
#define ENGINE_DEBUG_INSPECTOR_PANEL_H_

namespace engine::ecs {
class Registry;
}

namespace engine::debug {

class ComponentInspectorRegistry;
class EntityHierarchyPanel;

/**
 * @class InspectorPanel
 * @brief UI panel that displays components for the selected entity
 *
 * @details
 * Monitors the selected entity in the EntityHierarchyPanel and
 * uses the ComponentInspectorRegistry to display relevant
 * debug accessors for that entity's components.
 */
class InspectorPanel {
 public:
  /**
   * @brief Construct an Inspector Panel
   * @param registry Global ECS registry
   * @param inspector_registry Registry of component inspectors
   * @param hierarchy_panel Reference to the hierarchy panel for selection state
   */
  InspectorPanel(engine::ecs::Registry& registry,
                 const ComponentInspectorRegistry& inspector_registry,
                 const EntityHierarchyPanel& hierarchy_panel);

  ~InspectorPanel() = default;

  InspectorPanel(const InspectorPanel&) = delete;
  InspectorPanel& operator=(const InspectorPanel&) = delete;

  /**
   * @brief Draw the inspector panel
   */
  void Draw();

 private:
  engine::ecs::Registry& registry_;
  const ComponentInspectorRegistry& inspector_registry_;
  const EntityHierarchyPanel& hierarchy_panel_;
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_INSPECTOR_PANEL_H_
