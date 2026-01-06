/**
 * @file inspector_panel.cpp
 * @brief Implementation of the InspectorPanel
 */

#include "engine/debug/inspector_panel.h"

#include <imgui.h>

#include <string>

#include "engine/debug/component_inspector_registry.h"
#include "engine/debug/entity_hierarchy_panel.h"
#include "engine/ecs/registry.h"

namespace engine::debug {

InspectorPanel::InspectorPanel(
    engine::ecs::Registry& registry,
    const ComponentInspectorRegistry& inspector_registry,
    const EntityHierarchyPanel& hierarchy_panel)
    : registry_(registry),
      inspector_registry_(inspector_registry),
      hierarchy_panel_(hierarchy_panel) {}

void InspectorPanel::Draw() {
  ImGui::BeginChild("Inspector", ImVec2(0, 0), true);

  auto selected_entity = hierarchy_panel_.selected_entity();

  if (!selected_entity.has_value()) {
    ImGui::TextDisabled("Select an entity to inspect");
    ImGui::EndChild();
    return;
  }

  ecs::EntityId entity = *selected_entity;
  ImGui::Text("Entity ID: %zu", static_cast<std::size_t>(entity));
  ImGui::Separator();

  for (const auto& [type_index, meta] : inspector_registry_.All()) {
    meta.draw_fn(registry_, entity);
  }

  ImGui::EndChild();
}

}  // namespace engine::debug
