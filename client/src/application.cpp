#include "application.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "engine/math/vector2.h"
#include "engine/render.h"
#include "engine/time/game_loop.h"
#include "logging.h"
#include "scene/connecting_scene.h"
#include "scene/disconnected_scene.h"
#include "scene/game_over_scene.h"
#include "scene/in_game_scene.h"
#include "scene/main_menu_scene.h"
#include "scene/pause_scene.h"

namespace client {

Application::Application(ClientConfig config)
    : config_(std::move(config)),
      transport_(),
      join_flow_(config_.player_name, config_.room_code) {}

int Application::Run() {
  ConfigureClientLogging(config_.log_level);
  LogLifecycle(engine::util::LogLevel::kInfo, "Starting R-Type client");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "target configured");

  engine::core::EngineRuntimeConfig runtime_config;
  runtime_config.window_config.title = "R-Type Client";
  runtime_config.window_config.size = engine::math::Vector2i(1280, 720);
  runtime_config.window_config.vsync = true;
  runtime_config.window_config.target_fps = 60;
  runtime_config.log_level = config_.log_level;

  engine_ = engine::core::EngineRuntime::Create(runtime_config);
  if (!engine_) {
    LogLifecycle(engine::util::LogLevel::kCritical,
                 "Failed to initialize engine runtime");
    return 1;
  }

  if (auto* audio = engine_->Audio()) {
    audio_manager_ = std::make_unique<AudioManager>(*audio);
    audio_manager_->LoadAssets();
    LogLifecycle(engine::util::LogLevel::kInfo, "Audio manager initialized");
  }

  auto& input = engine_->Input();
  input.BindKey("Confirm", engine::input::Key::kEnter);
  input.BindKey("Cancel", engine::input::Key::kEscape);
  input.BindKey("Quit", engine::input::Key::kQ);

  auto& runtime_config_store = engine_->Config();
  runtime_config_store.Set("client.host", config_.host);
  runtime_config_store.Set("client.port", std::to_string(config_.port));
  runtime_config_store.Set("client.debug", config_.debug ? "true" : "false");
  runtime_config_store.Set(
      "client.log_level",
      std::string(engine::util::ToString(config_.log_level)));
  runtime_config_store.Set("client.player_name", config_.player_name);
  runtime_config_store.Set("client.room_code", config_.room_code);

  LogLifecycle(engine::util::LogLevel::kInfo, "Engine runtime ready");
  LogLifecycle(engine::util::LogLevel::kDebug, "Entering main loop");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  const auto transport_error = transport_.Start(config_.host, config_.port);
  if (transport_error) {
    LogLifecycle(engine::util::LogLevel::kError,
                 std::string("Failed to start network transport: ") +
                     transport_error.message());
    return 1;
  }

  SwitchScene(std::make_unique<MainMenuScene>(*this));

  engine::time::VariableTimestepLoop loop(
      static_cast<float>(runtime_config.window_config.target_fps));

  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });
  transport_.Stop();
  LogLifecycle(engine::util::LogLevel::kInfo, "Client shutdown complete");
  return 0;
}

void Application::SwitchScene(std::unique_ptr<Scene> scene) {
  current_scene_ = std::move(scene);
}

void Application::StartConnection() {
  join_flow_.Begin(transport_);
  SwitchScene(std::make_unique<ConnectingScene>(*this));
}

void Application::OnConnected() {
  SwitchScene(std::make_unique<InGameScene>(*this));
}

void Application::OnConnectionFailed(const std::string& reason) {
  SwitchScene(std::make_unique<DisconnectedScene>(*this, reason));
}

void Application::OnGameStart() {}

void Application::OnGamePause() {
  SwitchScene(std::make_unique<PauseScene>(*this));
}

void Application::OnGameResume() {
  SwitchScene(std::make_unique<InGameScene>(*this));
}

void Application::OnGameOver() {
  SwitchScene(std::make_unique<GameOverScene>(*this));
}

void Application::OnDisconnect() {
  SwitchScene(std::make_unique<DisconnectedScene>(*this, "Connection lost"));
}

void Application::OnQuitToMenu() {
  SwitchScene(std::make_unique<MainMenuScene>(*this));
}

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "Engine pump stopped the client loop");
    return false;
  }

  if (current_scene_) {
    current_scene_->Update(dt);
  }

  auto& window = engine_->Window();
  auto& context = window.GetRenderContext();
  auto& renderer = context.Get2DRenderer();

  const float fps = dt.as_seconds() > 0.0f ? 1.0f / dt.as_seconds() : 0.0f;

  std::ostringstream hud;
  hud << std::fixed << std::setprecision(1) << "FPS: " << fps;

  context.BeginFrame();
  context.Clear(engine::render::Color::FromBytes(12, 12, 16));

  if (current_scene_) {
    current_scene_->Draw(renderer);
  }

  renderer.DrawText(hud.str(), {24.0f, 24.0f}, 20.0f,
                    engine::render::Color::FromBytes(200, 200, 200));

  context.EndFrame();
  return true;
}

}  // namespace client
