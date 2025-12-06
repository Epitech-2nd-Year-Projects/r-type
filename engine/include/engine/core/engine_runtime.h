#ifndef ENGINE_ENGINE_CORE_ENGINE_RUNTIME_H_
#define ENGINE_ENGINE_CORE_ENGINE_RUNTIME_H_

#include <functional>
#include <memory>

#include "engine/audio/audio_engine.h"
#include "engine/input.h"
#include "engine/render/window.h"
#include "engine/util/config.h"
#include "engine/util/logging.h"

namespace engine::render {
class Renderer2D;
class WindowBackend;
}  // namespace engine::render

namespace engine::core {

/**
 * @brief Parameters used to bootstrap engine subsystems
 */
struct EngineRuntimeConfig {
  render::WindowConfig window_config{};
  util::LogLevel log_level{util::LogLevel::kInfo};
  bool enable_audio{true};
  bool colorize_logs{true};
  std::function<std::unique_ptr<render::WindowBackend>()>
      window_backend_factory;
};

/**
 * @brief Owning wrapper around rendering, input, audio and configuration
 */
class EngineRuntime {
 public:
  EngineRuntime(const EngineRuntime&) = delete;
  EngineRuntime& operator=(const EngineRuntime&) = delete;
  EngineRuntime(EngineRuntime&&) noexcept = delete;
  EngineRuntime& operator=(EngineRuntime&&) noexcept = delete;
  ~EngineRuntime();

  /**
   * @brief Construct an EngineRuntime instance
   */
  static std::unique_ptr<EngineRuntime> Create(
      const EngineRuntimeConfig& config);

  /**
   * @brief Mutable access to runtime configuration
   */
  [[nodiscard]] util::Configuration& Config();

  /**
   * @brief Access the process logger configured for the engine
   */
  [[nodiscard]] util::Logger& Logger();

  /**
   * @brief Retrieve the shared input manager
   */
  [[nodiscard]] input::InputManager& Input();

  /**
   * @brief Access the active window
   */
  [[nodiscard]] render::Window& Window();

  /**
   * @brief Access the active render context
   */
  [[nodiscard]] render::RenderContext& RenderContext();

  /**
   * @brief Access the 2D renderer bound to the active context
   */
  [[nodiscard]] render::Renderer2D& Renderer();

  /**
   * @brief Access the audio engine when enabled
   */
  [[nodiscard]] audio::AudioEngine* Audio();

  /**
   * @brief Pump OS events and update streaming subsystems
   * @return false when shutdown is requested
   */
  bool Pump();

 private:
  EngineRuntime();

  void Initialize(const EngineRuntimeConfig& config);

  util::Configuration config_;
  util::Logger* logger_{nullptr};
  input::InputManager input_;
  std::unique_ptr<render::WindowBackend> window_backend_;
  std::unique_ptr<render::Window> window_;
  std::unique_ptr<audio::AudioEngine> audio_;
};

}  // namespace engine::core

#endif  // ENGINE_ENGINE_CORE_ENGINE_RUNTIME_H_
