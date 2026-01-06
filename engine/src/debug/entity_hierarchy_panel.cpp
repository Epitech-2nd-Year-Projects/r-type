#include "engine/debug/entity_hierarchy_panel.h"

#include <imgui.h>

#include <string>

namespace engine::debug {

EntityHierarchyPanel::EntityHierarchyPanel(const ecs::Registry& registry)
    : registry_(registry) {}

void EntityHierarchyPanel::Draw() {
  ImGui::BeginChild("Hierarchy", ImVec2(0, 0), true);

  const auto& entities = registry_.GetEntities();
  std::string label;

  for (const auto& entity : entities) {
    label = "Entity " + std::to_string(static_cast<std::size_t>(entity));

    bool is_selected =
        selected_entity_.has_value() && selected_entity_.value() == entity;

    if (ImGui::Selectable(label.c_str(), is_selected)) {
      selected_entity_ = entity;
    }

    if (is_selected) {
      ImGui::SetItemDefaultFocus();
    }
  }

  ImGui::EndChild();
}

std::optional<ecs::EntityId> EntityHierarchyPanel::selected_entity() const {
  return selected_entity_;
}

void EntityHierarchyPanel::ClearSelection() { selected_entity_.reset(); }

}  // namespace engine::debug
