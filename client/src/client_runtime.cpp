#include "client_runtime.h"

#include <iomanip>
#include <sstream>
#include <utility>

#include "constants/client_constants.h"
#include "ecs/components.h"
#include "ecs/render_debug.h"
#include "ecs/render_system.h"
#include "engine/app/engine_runtime.h"
#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render.h"
#include "logging.h"
#include "render/parallax_background.h"
#include "scene/scene.h"

namespace client {

ClientRuntime::ClientRuntime() = default;

ClientRuntime::~ClientRuntime() = default;

bool ClientRuntime::Initialize(const ClientConfig& config) {
  engine::app::EngineRuntimeConfig runtime_config;
  runtime_config.window_config.title =
      std::string(constants::client::kWindowTitle);
  runtime_config.window_config.size = constants::client::kBaseResolution;
  runtime_config.window_config.resizable = constants::client::kWindowResizable;
  runtime_config.window_config.vsync = constants::client::kWindowVsync;
  runtime_config.window_config.target_fps = constants::client::kTargetFps;
  runtime_config.log_level = config.log_level;
  runtime_config.window_backend_factory = engine::render::CreateRaylibBackend;

  const float aspect_ratio =
      static_cast<float>(runtime_config.window_config.size.x) /
      static_cast<float>(runtime_config.window_config.size.y);
  std::ostringstream window_info;
  window_info << "Window " << runtime_config.window_config.size.x << 'x'
              << runtime_config.window_config.size.y << " (" << std::fixed
              << std::setprecision(2) << aspect_ratio << ":1) raylib backend";
  LogLifecycle(engine::util::LogLevel::kInfo, window_info.str());

  engine_ = engine::app::EngineRuntime::Create(runtime_config);
  if (!engine_) {
    LogLifecycle(engine::util::LogLevel::kCritical,
                 "Failed to initialize engine runtime");
    return false;
  }

  background_ = std::make_unique<ParallaxBackground>(engine_->Renderer());
  LogLifecycle(engine::util::LogLevel::kInfo, "Engine runtime ready");
  return true;
}

void ClientRuntime::AttachWorld(engine::ecs::Registry& registry) {
  if (!engine_) {
    return;
  }
  render_system_ =
      std::make_unique<ecs::RenderSystem>(registry, engine_->Renderer());
  render_debug_ =
      std::make_unique<ecs::RenderDebug>(registry, engine_->Renderer());
}

bool ClientRuntime::Pump() { return engine_ && engine_->Pump(); }

void ClientRuntime::RenderFrame(engine::time::TimeDelta dt, ClientState state,
                                const std::shared_ptr<Scene>& scene,
                                const engine::ecs::Registry& registry,
                                std::optional<float> latency_ms) {
  if (!engine_) {
    return;
  }

  UpdateDebugToggle();
  UpdateProfilingOverlay(dt, registry, latency_ms);

  auto& context = engine_->RenderContext();
  auto& renderer = engine_->Renderer();

  context.BeginFrame();
  context.Clear(constants::client::kClearColor);

  const bool render_gameplay =
      state == ClientState::kInGame || state == ClientState::kGameOver;

  if (background_ && render_gameplay) {
    const auto window_size = engine_->Window().GetSize();
    background_->Update(dt, {static_cast<float>(window_size.x),
                             static_cast<float>(window_size.y)});
    background_->Draw();
  }

  if (render_system_ && render_gameplay) {
    render_system_->Render();
  }
  if (render_debug_ && render_gameplay) {
    render_debug_->Draw();
  }

  if (scene) {
    scene->Draw(renderer);
  }

  profiling_overlay_.Draw(renderer, engine_->Window().GetSize());
  context.EndFrame();
}

engine::render::Renderer2D& ClientRuntime::Renderer() {
  return engine_->Renderer();
}

engine::input::InputManager& ClientRuntime::Input() { return engine_->Input(); }

engine::render::Window& ClientRuntime::Window() { return engine_->Window(); }

engine::render::RenderContext& ClientRuntime::RenderContext() {
  return engine_->RenderContext();
}

std::shared_ptr<engine::audio::AudioEngine> ClientRuntime::Audio() {
  if (!engine_) {
    return {};
  }
  return engine_->Audio();
}

engine::util::Configuration& ClientRuntime::Config() {
  return engine_->Config();
}

void ClientRuntime::RequestClose() {
  if (engine_) {
    engine_->Window().RequestClose();
  }
}

void ClientRuntime::ResetWorld() {
  if (render_system_) {
    render_system_->Reset();
  }
}

void ClientRuntime::UpdateProfilingOverlay(
    engine::time::TimeDelta dt, const engine::ecs::Registry& registry,
    std::optional<float> latency_ms) {
  profiling_overlay_.Update(dt);
  profiling_overlay_.UpdateEntityCount(RenderableEntityCount(registry));
  if (latency_ms.has_value()) {
    profiling_overlay_.UpdateLatency(*latency_ms);
    profiling_overlay_.UpdatePacketReceived();
  }
}

void ClientRuntime::UpdateDebugToggle() {
  if (!engine_) {
    return;
  }

  auto& input = engine_->Input();
  const bool pressed = input.IsKeyDown(engine::input::Key::kF3);
  if (pressed && !debug_toggle_pressed_) {
    profiling_overlay_.Toggle();
    if (render_debug_) {
      render_debug_->SetEnabled(profiling_overlay_.enabled());
    }
  }
  debug_toggle_pressed_ = pressed;
}

std::size_t ClientRuntime::RenderableEntityCount(
    const engine::ecs::Registry& registry) const {
  const auto& sprites = registry.GetComponents<ecs::SpriteComponent>();
  std::size_t count = 0;
  for (const auto& sprite : sprites) {
    if (sprite.has_value() && sprite->visible) {
      ++count;
    }
  }
  return count;
}

}  // namespace client
