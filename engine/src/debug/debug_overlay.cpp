#include "engine/debug/debug_overlay.h"

#include <imgui.h>

namespace engine::debug {

DebugOverlay::DebugOverlay(engine::ecs::Registry& registry,
                           const ComponentInspectorRegistry& inspector_registry)
    : hierarchy_panel_(registry),
      inspector_panel_(registry, inspector_registry, hierarchy_panel_) {}

void DebugOverlay::RegisterDebugToggle(std::string label, bool* value) {
  toggles_.emplace_back(std::move(label), value);
}

void DebugOverlay::Draw() {
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Engine Debugger", nullptr, ImGuiWindowFlags_MenuBar)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("View")) {
      for (auto& [label, value] : toggles_) {
        if (value) {
          ImGui::MenuItem(label.c_str(), nullptr, value);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::Columns(2, "DebugColumns", true);
  hierarchy_panel_.Draw();
  ImGui::NextColumn();
  inspector_panel_.Draw();
  ImGui::Columns(1);

  ImGui::End();
}

}  // namespace engine::debug
