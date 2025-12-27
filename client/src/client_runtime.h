/**
 * @file client_runtime.h
 * @brief Client runtime wrapper for engine systems
 */

#ifndef CLIENT_CLIENT_RUNTIME_H_
#define CLIENT_CLIENT_RUNTIME_H_

#include <cstddef>
#include <memory>
#include <optional>

#include "client_config.h"
#include "client_state.h"
#include "debug_overlay.h"
#include "engine/time/time_delta.h"

namespace engine::app {
class EngineRuntime;
}  // namespace engine::app

namespace engine::audio {
class AudioEngine;
}  // namespace engine::audio

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace engine::input {
class InputManager;
}  // namespace engine::input

namespace engine::render {
class RenderContext;
class Renderer2D;
class Window;
}  // namespace engine::render

namespace engine::util {
class Configuration;
}  // namespace engine::util

namespace client {

class ParallaxBackground;
class Scene;

namespace ecs {
class RenderSystem;
}  // namespace ecs

/**
 * @brief Client runtime for engine boot and frame rendering
 */
class ClientRuntime {
 public:
  /**
   * @brief Construct an empty runtime
   */
  ClientRuntime();

  /**
   * @brief Destroy the runtime
   */
  ~ClientRuntime();

  ClientRuntime(const ClientRuntime&) = delete;
  ClientRuntime& operator=(const ClientRuntime&) = delete;
  ClientRuntime(ClientRuntime&&) = delete;
  ClientRuntime& operator=(ClientRuntime&&) = delete;

  /**
   * @brief Initialize engine systems
   * @param config Client configuration
   * @return True when initialization succeeds
   */
  bool Initialize(const ClientConfig& config);

  /**
   * @brief Attach world systems for rendering
   * @param registry World registry
   */
  void AttachWorld(engine::ecs::Registry& registry);

  /**
   * @brief Pump engine systems
   * @return False when shutdown is requested
   */
  bool Pump();

  /**
   * @brief Render a full frame
   * @param dt Frame delta
   * @param state Current client state
   * @param scene Active scene
   * @param registry World registry
   * @param latency_ms Latest latency sample
   */
  void RenderFrame(engine::time::TimeDelta dt, ClientState state,
                   const std::shared_ptr<Scene>& scene,
                   const engine::ecs::Registry& registry,
                   std::optional<float> latency_ms);

  /**
   * @brief Access the renderer
   */
  engine::render::Renderer2D& Renderer();

  /**
   * @brief Access the input manager
   */
  engine::input::InputManager& Input();

  /**
   * @brief Access the active window
   */
  engine::render::Window& Window();

  /**
   * @brief Access the render context
   */
  engine::render::RenderContext& RenderContext();

  /**
   * @brief Access the audio engine when available
   */
  std::shared_ptr<engine::audio::AudioEngine> Audio();

  /**
   * @brief Access runtime configuration
   */
  engine::util::Configuration& Config();

  /**
   * @brief Request a window close
   */
  void RequestClose();

  /**
   * @brief Reset rendering state for a new session
   */
  void ResetWorld();

 private:
  void UpdateDebugOverlay(engine::time::TimeDelta dt,
                          const engine::ecs::Registry& registry,
                          std::optional<float> latency_ms);
  void UpdateDebugToggle();
  std::size_t RenderableEntityCount(
      const engine::ecs::Registry& registry) const;

  std::unique_ptr<engine::app::EngineRuntime> engine_;
  std::unique_ptr<ecs::RenderSystem> render_system_;
  std::unique_ptr<ParallaxBackground> background_;
  DebugOverlay debug_overlay_{};
  bool debug_toggle_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_CLIENT_RUNTIME_H_
