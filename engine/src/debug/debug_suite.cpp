#include "engine/debug/debug_suite.h"

#include <imgui.h>

#include "engine/console/console_overlay.h"
#include "engine/debug/network_debugger.h"
#include "engine/profiling/profiling_overlay.h"

namespace engine::debug {

DebugSuite::DebugSuite(ecs::Registry& registry,
                       const ComponentInspectorRegistry& inspector_registry)
    : hierarchy_panel_(registry),
      inspector_panel_(registry, inspector_registry, hierarchy_panel_) {}

void DebugSuite::SetNetworkDebugger(
    std::reference_wrapper<NetworkDebugger> network_debugger) {
  network_debugger_ = network_debugger;
}

void DebugSuite::SetConsoleOverlay(
    std::reference_wrapper<engine::console::ConsoleOverlay> console_overlay) {
  console_overlay_ = console_overlay;
}

void DebugSuite::SetProfilingOverlay(
    std::reference_wrapper<engine::profiling::ProfilingOverlay>
        profiling_overlay) {
  profiling_overlay_ = profiling_overlay;
}

void DebugSuite::RegisterGizmo(std::string name,
                               std::reference_wrapper<bool> value) {
  gizmos_.emplace_back(std::move(name), value);
}

void DebugSuite::Draw() {
  DrawMainMenuBar();

  if (config_.show_hierarchy || config_.show_inspector) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Engine Debugger", nullptr, ImGuiWindowFlags_MenuBar)) {
      if (config_.show_hierarchy && config_.show_inspector) {
        ImGui::Columns(2, "DebugColumns", true);
        hierarchy_panel_.Draw();
        ImGui::NextColumn();
        inspector_panel_.Draw();
        ImGui::Columns(1);
      } else if (config_.show_hierarchy) {
        hierarchy_panel_.Draw();
      } else if (config_.show_inspector) {
        inspector_panel_.Draw();
      }
    }
    ImGui::End();
  }

  if (config_.show_network && network_debugger_.has_value()) {
    network_debugger_->get().DrawPanel();
  }
  if (config_.show_demo_window) {
    ImGui::ShowDemoWindow(&config_.show_demo_window);
  }
}

void DebugSuite::DrawMainMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Hierarchy", nullptr, &config_.show_hierarchy);
      ImGui::MenuItem("Inspector", nullptr, &config_.show_inspector);
      ImGui::MenuItem("Network Debugger", nullptr, &config_.show_network);
      if (profiling_overlay_.has_value()) {
        bool profiler_enabled = profiling_overlay_->get().enabled();
        if (ImGui::MenuItem("Profiler Overlay", nullptr, &profiler_enabled)) {
          profiling_overlay_->get().SetEnabled(profiler_enabled);
          config_.show_profiler = profiler_enabled;
        }
      } else {
        ImGui::MenuItem("Profiler Overlay (Unavailable)", nullptr, false,
                        false);
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Gizmos")) {
      if (gizmos_.empty()) {
        ImGui::TextDisabled("No Gizmos Registered");
      } else {
        for (auto& [name, value_ref] : gizmos_) {
          ImGui::MenuItem(name.c_str(), nullptr, &value_ref.get());
        }
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

}  // namespace engine::debug
