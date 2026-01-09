#include "client_runtime.h"

#include <imgui.h>
#include <raylib.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "constants/client_constants.h"
#include "debug/inspector_registration.h"
#include "ecs/components.h"
#include "ecs/render_debug.h"
#include "ecs/render_system.h"
#include "engine/app/engine_runtime.h"
#include "engine/console/console.h"
#include "engine/console/console_overlay.h"
#include "engine/debug/component_inspector_registry.h"
#include "engine/debug/debug_suite.h"
#include "engine/debug/network_debugger.h"
#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render.h"
#include "logging.h"
#include "render/parallax_background.h"
#include "scene/scene.h"
#include "systems/debug_path_system.h"

namespace {

unsigned char ToByte(float value) {
  if (value < 0.0f) {
    value = 0.0f;
  } else if (value > 1.0f) {
    value = 1.0f;
  }
  return static_cast<unsigned char>(value * 255.0f + 0.5f);
}

::Color ToRaylibColor(const engine::render::Color& color) {
  return ::Color{ToByte(color.r), ToByte(color.g), ToByte(color.b),
                 ToByte(color.a)};
}

}  // namespace

namespace client {

struct ClientRuntime::BloomResources {
  ~BloomResources() { Reset(); }

  bool Initialize(std::string_view shader_path) {
    std::string path(shader_path);
    shader = ::LoadShader(nullptr, path.c_str());
    shader_ready = shader.id != 0;
    if (shader_ready) {
      resolution_loc = ::GetShaderLocation(shader, "resolution");
      threshold_loc = ::GetShaderLocation(shader, "threshold");
      knee_loc = ::GetShaderLocation(shader, "knee");
      intensity_loc = ::GetShaderLocation(shader, "intensity");
      ApplySettings();
    }
    return shader_ready;
  }

  void EnsureTarget(const engine::math::Vector2i& size) {
    if (!shader_ready || size.x <= 0 || size.y <= 0) {
      return;
    }
    if (target.id == 0 || target_size.x != size.x || target_size.y != size.y) {
      if (target.id != 0) {
        ::UnloadRenderTexture(target);
      }
      target = ::LoadRenderTexture(size.x, size.y);
      target_size = size;
    }
    if (resolution_loc >= 0) {
      const ::Vector2 resolution{
          static_cast<float>(target_size.x),
          static_cast<float>(target_size.y),
      };
      ::SetShaderValue(shader, resolution_loc, &resolution,
                       SHADER_UNIFORM_VEC2);
    }
    ApplySettings();
  }

  bool Ready() const { return shader_ready && target.id != 0; }

  void BeginCapture(const engine::render::Color& clear_color) {
    ::BeginTextureMode(target);
    ::ClearBackground(ToRaylibColor(clear_color));
  }

  void EndCapture() { ::EndTextureMode(); }

  void DrawCaptured(const engine::math::Vector2i& window_size) const {
    if (!Ready()) {
      return;
    }
    const float width = static_cast<float>(window_size.x);
    const float height = static_cast<float>(window_size.y);
    if (width <= 0.0f || height <= 0.0f) {
      return;
    }
    const ::Rectangle source{0.0f, 0.0f,
                             static_cast<float>(target.texture.width),
                             -static_cast<float>(target.texture.height)};
    const ::Rectangle dest{0.0f, 0.0f, width, height};
    ::DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
  }

  void DrawBloom(const engine::math::Vector2i& window_size) const {
    if (!Ready()) {
      return;
    }
    const float width = static_cast<float>(window_size.x);
    const float height = static_cast<float>(window_size.y);
    if (width <= 0.0f || height <= 0.0f) {
      return;
    }
    const ::Rectangle source{0.0f, 0.0f,
                             static_cast<float>(target.texture.width),
                             -static_cast<float>(target.texture.height)};
    const ::Rectangle dest{0.0f, 0.0f, width, height};
    ::BeginBlendMode(BLEND_ADDITIVE);
    ::BeginShaderMode(shader);
    ::DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
    ::EndShaderMode();
    ::EndBlendMode();
  }

  void ApplySettings() {
    if (!shader_ready) {
      return;
    }
    const float threshold = constants::client::kBloomThreshold;
    const float knee = constants::client::kBloomKnee;
    const float intensity = constants::client::kBloomIntensity;
    if (threshold_loc >= 0) {
      ::SetShaderValue(shader, threshold_loc, &threshold, SHADER_UNIFORM_FLOAT);
    }
    if (knee_loc >= 0) {
      ::SetShaderValue(shader, knee_loc, &knee, SHADER_UNIFORM_FLOAT);
    }
    if (intensity_loc >= 0) {
      ::SetShaderValue(shader, intensity_loc, &intensity, SHADER_UNIFORM_FLOAT);
    }
  }

  void Reset() {
    if (::IsWindowReady()) {
      if (target.id != 0) {
        ::UnloadRenderTexture(target);
      }
      if (shader.id != 0) {
        ::UnloadShader(shader);
      }
    }
    target = {};
    target_size = {};
    shader = {};
    shader_ready = false;
    resolution_loc = -1;
  }

  Shader shader{};
  RenderTexture2D target{};
  engine::math::Vector2i target_size{};
  int resolution_loc{-1};
  int threshold_loc{-1};
  int knee_loc{-1};
  int intensity_loc{-1};
  bool shader_ready{false};
};

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
  imgui_ = std::make_unique<engine::debug::ImGuiIntegration>();
  imgui_->SetEnabled(false);
  component_registry_ =
      std::make_unique<engine::debug::ComponentInspectorRegistry>();
  network_debugger_ = std::make_unique<engine::debug::NetworkDebugger>();
  client::debug::RegisterClientInspectors(*component_registry_);

  frame_profiler_ = std::make_unique<engine::profiling::FrameProfiler>();
  profiling_overlay_.SetFrameProfiler(*frame_profiler_);

  bloom_ = std::make_unique<BloomResources>();
  if (!bloom_->Initialize(constants::client::kBloomShaderPath)) {
    LogLifecycle(engine::util::LogLevel::kWarn, "Bloom shader failed to load");
  }
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
  debug_path_system_ =
      std::make_unique<systems::DebugPathSystem>(registry, *render_debug_);

  debug_suite_ = std::make_unique<engine::debug::DebugSuite>(
      registry, *component_registry_);

  if (network_debugger_) {
    debug_suite_->SetNetworkDebugger(std::ref(*network_debugger_));
  }
  if (engine_) {
    debug_suite_->SetConsoleOverlay(std::ref(engine_->ConsoleOverlay()));
  }
  debug_suite_->SetProfilingOverlay(std::ref(profiling_overlay_));

  if (render_debug_) {
    debug_suite_->RegisterGizmo("Colliders (Red)",
                                std::ref(render_debug_->show_colliders));
    debug_suite_->RegisterGizmo("Sprites (Blue)",
                                std::ref(render_debug_->show_sprite_bounds));
    debug_suite_->RegisterGizmo("Velocity (Yellow)",
                                std::ref(render_debug_->show_velocity));
    debug_suite_->RegisterGizmo("AI Paths",
                                std::ref(render_debug_->show_ai_paths));
  }
}

bool ClientRuntime::Pump() { return engine_ && engine_->Pump(); }

void ClientRuntime::RenderFrame(engine::time::TimeDelta dt, ClientState state,
                                const std::shared_ptr<Scene>& scene,
                                const engine::ecs::Registry& registry,
                                std::optional<float> latency_ms) {
  if (!engine_) {
    return;
  }

  frame_profiler_->RecordFrame(dt);

  UpdateDebugToggle();
  UpdateImGuiToggle();
  UpdateProfilingOverlay(dt, registry, latency_ms);
  UpdateConsoleOverlay(dt);

  auto& context = engine_->RenderContext();
  auto& renderer = engine_->Renderer();
  const auto window_size = engine_->Window().GetSize();

  context.BeginFrame();

  if (imgui_ && imgui_->enabled()) {
    imgui_->BeginFrame();
    if (debug_suite_) {
      debug_suite_->Draw();
    }
  }

  context.Clear(constants::client::kClearColor);

  const bool render_gameplay = state == ClientState::kInGame ||
                               state == ClientState::kPaused ||
                               state == ClientState::kGameOver;
  const bool render_with_bloom = !render_gameplay && bloom_;

  if (background_ && render_gameplay) {
    background_->Update(dt, {static_cast<float>(window_size.x),
                             static_cast<float>(window_size.y)});
    background_->Draw();
  }

  if (render_system_ && render_gameplay) {
    render_system_->Render();
  }
  if (render_debug_ && render_gameplay) {
    if (debug_path_system_) {
      debug_path_system_->Update(dt);
    }
    render_debug_->Draw();
  }

  if (render_with_bloom) {
    bloom_->EnsureTarget(window_size);
    if (bloom_->Ready()) {
      if (scene) {
        scene->DrawBackground(renderer);
      }
      bloom_->BeginCapture(engine::render::Color::Transparent());
      if (scene) {
        scene->DrawForeground(renderer);
      }
      bloom_->EndCapture();
      bloom_->DrawCaptured(window_size);
      bloom_->DrawBloom(window_size);
    } else if (scene) {
      scene->Draw(renderer);
    }
  } else if (scene) {
    scene->Draw(renderer);
  }

  profiling_overlay_.Draw(renderer, engine_->Window().GetSize());
  engine_->ConsoleOverlay().Draw(renderer, engine_->Window().GetSize());

  if (imgui_ && imgui_->enabled()) {
    imgui_->EndFrame();
  }

  profiling_overlay_.Draw(renderer, window_size);
  engine_->ConsoleOverlay().Draw(renderer, window_size);
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

engine::console::Console& ClientRuntime::Console() {
  return engine_->Console();
}

engine::console::ConsoleOverlay& ClientRuntime::ConsoleOverlay() {
  return engine_->ConsoleOverlay();
}

bool ClientRuntime::IsConsoleOpen() const {
  return engine_ && engine_->ConsoleOverlay().IsOpen();
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

void ClientRuntime::UpdateImGuiToggle() {
  if (!engine_ || !imgui_) {
    return;
  }

  auto& input = engine_->Input();
  const bool pressed = input.IsKeyDown(engine::input::Key::kF2);
  if (pressed && !imgui_toggle_pressed_) {
    imgui_->Toggle();
  }
  imgui_toggle_pressed_ = pressed;
}

void ClientRuntime::UpdateConsoleOverlay(engine::time::TimeDelta dt) {
  if (!engine_) {
    return;
  }
  engine_->ConsoleOverlay().Update(dt, engine_->Input());
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

engine::debug::NetworkDebugger& ClientRuntime::NetworkDebugger() {
  return *network_debugger_;
}

}  // namespace client
