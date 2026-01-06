/**
 * @file entity_hierarchy_panel.h
 * @brief ImGui panel for displaying and selecting entities
 * @version 1.0.0
 *
 * @details
 * Lists all active entities in the registry. Allows selecting an entity
 * for inspection.
 */

#ifndef ENGINE_DEBUG_ENTITY_HIERARCHY_PANEL_H_
#define ENGINE_DEBUG_ENTITY_HIERARCHY_PANEL_H_

#include <optional>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"

namespace engine::debug {

class EntityHierarchyPanel {
 public:
  /**
   * @brief Construct the panel with a reference to the registry
   * @param registry Reference to the main ECS registry
   */
  explicit EntityHierarchyPanel(const ecs::Registry& registry);
  ~EntityHierarchyPanel() = default;

  /**
   * @brief Draw the ImGui panel
   *
   * @details
   * Must be called inside an ImGui frame (between BeginFrame and EndFrame).
   * Creates a window named "Entity Hierarchy".
   */
  void Draw();

  /**
   * @brief Get the currently selected entity
   * @return Optional containing the selected EntityId, or nullopt
   */
  [[nodiscard]] std::optional<ecs::EntityId> selected_entity() const;

  /**
   * @brief Clear the current selection
   */
  void ClearSelection();

 private:
  const ecs::Registry& registry_;
  std::optional<ecs::EntityId> selected_entity_;
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_ENTITY_HIERARCHY_PANEL_H_
