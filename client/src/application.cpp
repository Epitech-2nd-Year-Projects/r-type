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

Application::Application(ClientConfig config) : config_(std::move(config)) {}

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

  auto& runtime_config_store = engine_->Config();
  runtime_config_store.Set("client.host", config_.host);
  runtime_config_store.Set("client.port", std::to_string(config_.port));
  runtime_config_store.Set("client.debug", config_.debug ? "true" : "false");
  runtime_config_store.Set(
      "client.log_level",
      std::string(engine::util::ToString(config_.log_level)));

  LogLifecycle(engine::util::LogLevel::kInfo, "Engine runtime ready");
  LogLifecycle(engine::util::LogLevel::kDebug, "Entering main loop");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  engine::time::VariableTimestepLoop loop(
      static_cast<float>(runtime_config.window_config.target_fps));

  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });
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

  const float fps = dt.as_seconds() > 0.0f ? 1.0f / dt.as_seconds() : 0.0f;

  std::ostringstream hud;
  hud << std::fixed << std::setprecision(1) << "FPS: " << fps;

  context.BeginFrame();
  context.Clear(engine::render::Color::FromBytes(12, 12, 16));

  renderer.DrawText("R-Type Client", {24.0f, 28.0f}, 28.0f,
                    engine::render::Color::White());
  renderer.DrawText(hud.str(), {24.0f, 64.0f}, 20.0f,
                    engine::render::Color::FromBytes(200, 200, 200));

  context.EndFrame();
  return true;
}

}  // namespace client
