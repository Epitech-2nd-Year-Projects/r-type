#include "client_runtime.h"

#include <imgui.h>
#include <raylib.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "constants/client_constants.h"
#include "debug/inspector_registration.h"
#include "ecs/archetype_registry.h"
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
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components/powerup_component.h"
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

struct RenderViewport {
  engine::math::Vector2f offset{};
  engine::math::Vector2f size{};
  float scale{1.0f};
};

RenderViewport ComputeRenderViewport(
    const engine::math::Vector2i& window_size,
    const engine::math::Vector2i& render_size) {
  RenderViewport viewport{};
  if (window_size.x <= 0 || window_size.y <= 0 || render_size.x <= 0 ||
      render_size.y <= 0) {
    return viewport;
  }

  const float window_width = static_cast<float>(window_size.x);
  const float window_height = static_cast<float>(window_size.y);
  const float render_width = static_cast<float>(render_size.x);
  const float render_height = static_cast<float>(render_size.y);
  const float scale_x = window_width / render_width;
  const float scale_y = window_height / render_height;

  viewport.scale = std::min(scale_x, scale_y);
  viewport.size = {render_width * viewport.scale,
                   render_height * viewport.scale};
  viewport.offset = {(window_width - viewport.size.x) * 0.5f,
                     (window_height - viewport.size.y) * 0.5f};
  return viewport;
}

}  // namespace

namespace client {

struct ClientRuntime::FrameResources {
  ~FrameResources() { Reset(); }

  void EnsureTarget(const engine::math::Vector2i& size) {
    if (size.x <= 0 || size.y <= 0) {
      return;
    }
    if (target.id == 0 || target_size.x != size.x || target_size.y != size.y) {
      if (target.id != 0) {
        ::UnloadRenderTexture(target);
      }
      target = ::LoadRenderTexture(size.x, size.y);
      target_size = size;
    }
  }

  bool Ready() const { return target.id != 0; }

  void BeginCapture(const engine::render::Color& clear_color) {
    ::BeginTextureMode(target);
    ::ClearBackground(ToRaylibColor(clear_color));
  }

  void EndCapture() { ::EndTextureMode(); }

  void DrawCaptured(const RenderViewport& viewport) const {
    if (!Ready()) {
      return;
    }
    const float width = viewport.size.x;
    const float height = viewport.size.y;
    if (width <= 0.0f || height <= 0.0f) {
      return;
    }
    const ::Rectangle source{0.0f, 0.0f,
                             static_cast<float>(target.texture.width),
                             -static_cast<float>(target.texture.height)};
    const ::Rectangle dest{viewport.offset.x, viewport.offset.y, width, height};
    ::DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
  }

  void Reset() {
    if (::IsWindowReady() && target.id != 0) {
      ::UnloadRenderTexture(target);
    }
    target = {};
    target_size = {};
  }

  RenderTexture2D target{};
  engine::math::Vector2i target_size{};
};

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

  void DrawCaptured(const RenderViewport& viewport) const {
    if (!Ready()) {
      return;
    }
    const float width = viewport.size.x;
    const float height = viewport.size.y;
    if (width <= 0.0f || height <= 0.0f) {
      return;
    }
    const ::Rectangle source{0.0f, 0.0f,
                             static_cast<float>(target.texture.width),
                             -static_cast<float>(target.texture.height)};
    const ::Rectangle dest{viewport.offset.x, viewport.offset.y, width, height};
    ::DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
  }

  void DrawBloom(const RenderViewport& viewport) const {
    if (!Ready()) {
      return;
    }
    const float width = viewport.size.x;
    const float height = viewport.size.y;
    if (width <= 0.0f || height <= 0.0f) {
      return;
    }
    const ::Rectangle source{0.0f, 0.0f,
                             static_cast<float>(target.texture.width),
                             -static_cast<float>(target.texture.height)};
    const ::Rectangle dest{viewport.offset.x, viewport.offset.y, width, height};
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
    if (threshold_loc >= 0) {
      ::SetShaderValue(shader, threshold_loc, &threshold, SHADER_UNIFORM_FLOAT);
    }
    if (knee_loc >= 0) {
      ::SetShaderValue(shader, knee_loc, &knee, SHADER_UNIFORM_FLOAT);
    }
    if (intensity_loc >= 0) {
      ::SetShaderValue(shader, intensity_loc, &current_intensity,
                       SHADER_UNIFORM_FLOAT);
    }
  }

  void SetIntensity(float multiplier) {
    current_intensity = constants::client::kBloomIntensity * multiplier;
    if (shader_ready && intensity_loc >= 0) {
      ::SetShaderValue(shader, intensity_loc, &current_intensity,
                       SHADER_UNIFORM_FLOAT);
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
  float current_intensity{constants::client::kBloomIntensity};
  bool shader_ready{false};
};

ClientRuntime::ClientRuntime() = default;

ClientRuntime::~ClientRuntime() = default;

bool ClientRuntime::Initialize(
    const ClientConfig& config,
    std::function<void(const protocol::CommandPayload&)>
        send_command_callback) {
  engine::app::EngineRuntimeConfig runtime_config;
  runtime_config.window_config.title =
      std::string(constants::client::kWindowTitle);
  runtime_config.window_config.size = {config.resolution_width,
                                       config.resolution_height};
  runtime_config.window_config.resizable = constants::client::kWindowResizable;
  runtime_config.window_config.fullscreen = config.fullscreen;
  runtime_config.window_config.vsync = config.vsync;
  runtime_config.window_config.target_fps = std::max(0, config.target_fps);
  SetRenderSize({constants::client::kBaseResolution.x,
                 constants::client::kBaseResolution.y});
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

  auto& script_engine = engine_->ScriptEngine();
  game_logic::BindRuntimeTypes(script_engine.LuaState(),
                               script_engine.GetPrefabFactory());
  script_engine.LoadScript("config/prefabs/powerups.lua");

  sol::table prefabs = script_engine.LuaState()["Prefabs"];
  if (prefabs.valid()) {
    for (const auto& pair : prefabs) {
      if (!pair.second.is<sol::table>()) continue;
      sol::table prefab = pair.second;

      sol::optional<sol::table> powerup = prefab["Powerup"];
      sol::optional<sol::table> sprite = prefab["Sprite"];

      if (powerup && sprite) {
        auto type = powerup->get<game_logic::components::PowerupType>("type");
        std::string texture = sprite->get<std::string>("texture");
        float width = sprite->get_or("width", 16.0f);
        float height = sprite->get_or("height", 16.0f);

        std::uint16_t type_code = 10 + static_cast<std::uint16_t>(type);
        ecs::ArchetypeRegistry::RegisterPowerupType(type_code, texture, width,
                                                    height);
        LogLifecycle(engine::util::LogLevel::kInfo,
                     "Registered dynamic powerup: " + texture);
      }
    }
  }

  script_engine.LoadScript("config/prefabs/enemies.lua");
  sol::table all_prefabs = script_engine.LuaState()["Prefabs"];
  if (all_prefabs.valid()) {
    std::vector<std::string> enemy_names;
    for (const auto& pair : all_prefabs) {
      if (!pair.second.is<sol::table>()) continue;
      sol::table t = pair.second;
      if (t["Tag"].get_or(std::string("")) == "Enemy") {
        enemy_names.push_back(pair.first.as<std::string>());
      }
    }
    std::sort(enemy_names.begin(), enemy_names.end());

    std::uint16_t id = 200;
    for (const auto& name : enemy_names) {
      sol::table t = all_prefabs[name];
      sol::optional<sol::table> sprite = t["Sprite"];
      if (sprite) {
        std::string texture = sprite->get<std::string>("texture");
        float width = sprite->get_or("width", 33.0f);
        float height = sprite->get_or("height", 33.0f);
        float frame_width = sprite->get_or("frame_width", width);
        float frame_height = sprite->get_or("frame_height", height);
        ecs::ArchetypeRegistry::RegisterEnemyType(id, texture, width, height,
                                                  frame_width, frame_height);
        LogLifecycle(engine::util::LogLevel::kInfo,
                     "Registered dynamic enemy: " + name + " -> " +
                         std::to_string(id) + " (Render: " +
                         std::to_string(width) + "x" + std::to_string(height) +
                         " Frame: " + std::to_string(frame_width) + "x" +
                         std::to_string(frame_height) + ")");
      }
      id++;
    }
  }

  script_engine.LoadScript("config/prefabs/players.lua");
  sol::table player_prefabs = script_engine.LuaState()["Prefabs"];
  if (player_prefabs.valid()) {
    sol::optional<sol::table> player_t = player_prefabs["Player"];
    if (player_t) {
      sol::optional<sol::table> sprite = (*player_t)["Sprite"];
      if (sprite) {
        float width = sprite->get_or("width", 26.0f);
        float height = sprite->get_or("height", 21.0f);
        float frame_width = sprite->get_or("frame_width", 26.0f);
        float frame_height = sprite->get_or("frame_height", 21.0f);
        ecs::ArchetypeRegistry::SetPlayerConfig(width, height, frame_width,
                                                frame_height);
        LogLifecycle(engine::util::LogLevel::kInfo,
                     "Configured player: Render " + std::to_string(width) +
                         "x" + std::to_string(height));
      }
    }
  }

  frame_resources_ = std::make_unique<FrameResources>();
  background_ = std::make_unique<ParallaxBackground>(engine_->Renderer());
  imgui_ = std::make_unique<engine::debug::ImGuiIntegration>();
  imgui_->SetEnabled(false);
  component_registry_ =
      std::make_unique<engine::debug::ComponentInspectorRegistry>();
  network_debugger_ = std::make_unique<engine::debug::NetworkDebugger>();
  client::debug::RegisterClientInspectors(*component_registry_, config.debug,
                                          std::move(send_command_callback));

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

bool ClientRuntime::Pump() {
  if (!engine_ || !engine_->Pump()) {
    return false;
  }
  SyncInputToRenderSize();
  return true;
}

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

  const bool render_gameplay = state == ClientState::kInGame ||
                               state == ClientState::kPaused ||
                               state == ClientState::kGameOver;
  const float bloom_intensity = scene ? scene->GetBloomIntensity() : 1.0f;
  const bool render_with_bloom =
      !render_gameplay && bloom_ && bloom_intensity > 0.0f;

  engine::math::Vector2i render_size = render_size_;
  if (render_size.x <= 0 || render_size.y <= 0) {
    render_size = window_size;
  }

  const auto render_viewport = ComputeRenderViewport(window_size, render_size);

  bool frame_ready = false;
  if (frame_resources_) {
    frame_resources_->EnsureTarget(render_size);
    frame_ready = frame_resources_->Ready();
  }

  if (background_) {
    background_->Update(dt, {static_cast<float>(render_size.x),
                             static_cast<float>(render_size.y)});
    background_->Draw();
  }

  if (render_with_bloom) {
    bloom_->SetIntensity(bloom_intensity);
    bloom_->EnsureTarget(window_size);
    if (bloom_->Ready()) {
      frame_resources_->BeginCapture(constants::client::kClearColor);
      if (scene) {
        scene->DrawBackground(renderer);
      }
      frame_resources_->EndCapture();

      bloom_->BeginCapture(engine::render::Color::Transparent());
      if (scene) {
        scene->DrawForeground(renderer);
      }
      bloom_->EndCapture();

      context.Clear(constants::client::kClearColor);
      frame_resources_->DrawCaptured(render_viewport);
      bloom_->DrawCaptured(render_viewport);
      bloom_->DrawBloom(render_viewport);
    } else if (frame_ready) {
      frame_resources_->BeginCapture(constants::client::kClearColor);
      if (scene) {
        scene->Draw(renderer);
      }
      frame_resources_->EndCapture();

      context.Clear(constants::client::kClearColor);
      frame_resources_->DrawCaptured(render_viewport);
    } else if (scene) {
      context.Clear(constants::client::kClearColor);
      scene->Draw(renderer);
    }
  } else {
    if (frame_ready) {
      frame_resources_->BeginCapture(constants::client::kClearColor);
      if (scene) {
        scene->Draw(renderer);
      }
      frame_resources_->EndCapture();

      context.Clear(constants::client::kClearColor);
      frame_resources_->DrawCaptured(render_viewport);
    } else if (scene) {
      context.Clear(constants::client::kClearColor);
      scene->Draw(renderer);
    }
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

engine::math::Vector2i ClientRuntime::RenderSize() const {
  return render_size_;
}

void ClientRuntime::SetRenderSize(const engine::math::Vector2i& size) {
  render_size_.x = std::max(1, size.x);
  render_size_.y = std::max(1, size.y);
}

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

void ClientRuntime::SyncInputToRenderSize() {
  if (!engine_) {
    return;
  }
  const auto window_size = engine_->Window().GetSize();
  if (window_size.x <= 0 || window_size.y <= 0) {
    return;
  }
  engine::math::Vector2i render_size = render_size_;
  if (render_size.x <= 0 || render_size.y <= 0) {
    render_size = window_size;
  }

  const auto viewport = ComputeRenderViewport(window_size, render_size);
  if (viewport.size.x <= 0.0f || viewport.size.y <= 0.0f ||
      viewport.scale <= 0.0f) {
    return;
  }

  auto& input = engine_->Input();
  const auto mouse_pos = input.GetMousePosition();
  const float mapped_x = (mouse_pos.x - viewport.offset.x) / viewport.scale;
  const float mapped_y = (mouse_pos.y - viewport.offset.y) / viewport.scale;
  input.SetMousePosition({mapped_x, mapped_y});
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
