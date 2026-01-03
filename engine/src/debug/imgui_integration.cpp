#include "engine/debug/imgui_integration.h"

#include "rlImGui.h"

namespace engine::debug {

ImGuiIntegration::ImGuiIntegration() : initialized_(true) {
  rlImGuiSetup(true);
}

ImGuiIntegration::~ImGuiIntegration() {
  if (initialized_) {
    rlImGuiShutdown();
    initialized_ = false;
  }
}

ImGuiIntegration::ImGuiIntegration(ImGuiIntegration&& other) noexcept
    : enabled_(other.enabled_), initialized_(other.initialized_) {
  other.initialized_ = false;
}

ImGuiIntegration& ImGuiIntegration::operator=(
    ImGuiIntegration&& other) noexcept {
  if (this != &other) {
    if (initialized_) {
      rlImGuiShutdown();
    }
    enabled_ = other.enabled_;
    initialized_ = other.initialized_;
    other.initialized_ = false;
  }
  return *this;
}

void ImGuiIntegration::BeginFrame() {
  if (!enabled_ || !initialized_) {
    return;
  }
  rlImGuiBegin();
}

void ImGuiIntegration::EndFrame() {
  if (!enabled_ || !initialized_) {
    return;
  }
  rlImGuiEnd();
}

}  // namespace engine::debug
