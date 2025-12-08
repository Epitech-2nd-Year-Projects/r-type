#include "application.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "engine/math/vector2.h"
#include "engine/render.h"
#include "engine/time/game_loop.h"
#include "logging.h"

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

  engine::time::VariableTimestepLoop loop(
      static_cast<float>(runtime_config.window_config.target_fps));

  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });
  transport_.Stop();
  LogLifecycle(engine::util::LogLevel::kInfo, "Client shutdown complete");
  return 0;
}

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "Engine pump stopped the client loop");
    return false;
  }

  auto& window = engine_->Window();
  auto& context = window.GetRenderContext();
  auto& renderer = context.Get2DRenderer();
  auto& input = engine_->Input(); // Handle Input
  const auto events = input.ConsumeEvents();
  bool confirm_pressed = false;
  bool cancel_pressed = false;
  bool quit_pressed = false;

  for (const auto& event : events) {
    if (event.type == engine::input::ActionEventType::kPressed) {
      if (event.action == "Confirm") confirm_pressed = true;
      else if (event.action == "Cancel") cancel_pressed = true;
      else if (event.action == "Quit") quit_pressed = true;
    }
  }

  // State Update
  switch (state_) {
    case GameState::kMainMenu:
      if (confirm_pressed) {
        state_ = GameState::kConnecting;
        join_flow_.Begin(transport_);
      }
      break;
    case GameState::kConnecting:
      join_flow_.Update(transport_);
      if (join_flow_.state() == JoinState::kConnected) {
        state_ = GameState::kInGame;
      } else if (join_flow_.state() == JoinState::kRefused) {
        // For now, just go back to main menu on failure
        state_ = GameState::kMainMenu;
      }
      break;
    case GameState::kInGame:
      if (cancel_pressed) {
        state_ = GameState::kPaused;
      }
      // Game logic running
      break;
    case GameState::kPaused:
      if (confirm_pressed || cancel_pressed) {
        state_ = GameState::kInGame;
      } else if (quit_pressed) {
        state_ = GameState::kMainMenu;
      }
      break;
  }

  const float fps = dt.as_seconds() > 0.0f ? 1.0f / dt.as_seconds() : 0.0f;

  std::ostringstream hud;
  hud << std::fixed << std::setprecision(1) << "FPS: " << fps;

  context.BeginFrame();
  context.Clear(engine::render::Color::FromBytes(12, 12, 16));

  renderer.DrawText(hud.str(), {24.0f, 24.0f}, 20.0f,
                    engine::render::Color::FromBytes(200, 200, 200));

  switch (state_) {
    case GameState::kMainMenu:
      renderer.DrawText("R-Type Client", {300.0f, 200.0f}, 48.0f,
                        engine::render::Color::White());
      renderer.DrawText("Press ENTER to Connect", {300.0f, 300.0f}, 24.0f,
                        engine::render::Color::White());
      break;
    case GameState::kConnecting:
      renderer.DrawText(join_flow_.status(), {300.0f, 300.0f}, 24.0f,
                        engine::render::Color::White());
      break;
    case GameState::kInGame:
      renderer.DrawText("In Game", {300.0f, 50.0f}, 32.0f,
                        engine::render::Color::White());
      if (const auto player_id = join_flow_.player_id()) {
        renderer.DrawText("Player ID: " + std::to_string(*player_id),
                          {300.0f, 100.0f}, 18.0f,
                          engine::render::Color::FromBytes(180, 220, 255));
      }
      break;
    case GameState::kPaused:
      renderer.DrawText("Paused", {300.0f, 200.0f}, 48.0f,
                        engine::render::Color::White());
      renderer.DrawText("Press ENTER to Resume", {300.0f, 300.0f}, 24.0f,
                        engine::render::Color::White());
      renderer.DrawText("Press Q to Quit to Main Menu", {300.0f, 350.0f}, 24.0f,
                        engine::render::Color::White());
      break;
  }

  context.EndFrame();
  return true;
}

}  // namespace client
