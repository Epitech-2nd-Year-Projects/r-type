#include "engine/debug/debug_overlay.h"

#include <imgui.h>

namespace engine::debug {

DebugOverlay::DebugOverlay(engine::ecs::Registry& registry,
                           const ComponentInspectorRegistry& inspector_registry)
    : hierarchy_panel_(registry),
      inspector_panel_(registry, inspector_registry, hierarchy_panel_) {}

void DebugOverlay::Draw() {
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Engine Debugger", nullptr, ImGuiWindowFlags_None)) {
    ImGui::End();
    return;
  }

  ImGui::Columns(2, "DebugColumns", true);
  hierarchy_panel_.Draw();
  ImGui::NextColumn();
  inspector_panel_.Draw();
  ImGui::Columns(1);

  ImGui::End();
}

}  // namespace engine::debug
