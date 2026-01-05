/**
 * @file imgui_integration.h
 * @brief RAII wrapper for Dear ImGui with Raylib backend
 * @version 1.0.0
 *
 * @details
 * Provides a lifecycle-managed ImGui integration using rlImGui.
 * The integration is conditionally compiled via RTYPE_DEBUG.
 */

#ifndef ENGINE_DEBUG_IMGUI_INTEGRATION_H_
#define ENGINE_DEBUG_IMGUI_INTEGRATION_H_

namespace engine::debug {

/**
 * @class ImGuiIntegration
 * @brief RAII wrapper for Dear ImGui lifecycle
 *
 * @details
 * Manages ImGui initialization and shutdown using RAII pattern.
 * Constructor sets up ImGui with dark theme, destructor performs cleanup.
 *
 * @section usage Usage Example
 * @code
 * ImGuiIntegration imgui;
 *
 * while (running) {
 *   imgui.BeginFrame();
 *   ImGui::ShowDemoWindow();
 *   imgui.EndFrame();
 * }
 * @endcode
 *
 * @note Requires a valid Raylib window context before construction.
 */
class ImGuiIntegration {
 public:
  /**
   * @brief Initialize ImGui with Raylib backend
   *
   * @details
   * Calls rlImGuiSetup with dark theme enabled.
   * Must be called after InitWindow().
   */
  ImGuiIntegration();

  /**
   * @brief Shutdown ImGui
   *
   * @details
   * Calls rlImGuiShutdown to release all ImGui resources.
   */
  ~ImGuiIntegration();

  ImGuiIntegration(const ImGuiIntegration&) = delete;
  ImGuiIntegration& operator=(const ImGuiIntegration&) = delete;

  /**
   * @brief Move constructor
   * @param other Source integration to move from
   */
  ImGuiIntegration(ImGuiIntegration&& other) noexcept;

  /**
   * @brief Move assignment operator
   * @param other Source integration to move from
   * @return Reference to this
   */
  ImGuiIntegration& operator=(ImGuiIntegration&& other) noexcept;

  /**
   * @brief Begin a new ImGui frame
   *
   * @details
   * Must be called at the start of each frame before any ImGui calls.
   * Does nothing if ImGui is disabled.
   */
  void BeginFrame();

  /**
   * @brief End the current ImGui frame and render
   *
   * @details
   * Must be called at the end of each frame after all ImGui calls.
   * Does nothing if ImGui is disabled.
   */
  void EndFrame();

  /**
   * @brief Enable or disable ImGui rendering
   * @param enabled True to enable, false to disable
   */
  void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }

  /**
   * @brief Check if ImGui is enabled
   * @return True if enabled
   */
  bool enabled() const noexcept { return enabled_; }

  /**
   * @brief Toggle ImGui enabled state
   */
  void Toggle() noexcept { enabled_ = !enabled_; }

 private:
  bool enabled_{true};
  bool initialized_{false};
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_IMGUI_INTEGRATION_H_
