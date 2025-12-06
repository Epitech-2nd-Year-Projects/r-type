#include "application.h"

#include <iomanip>
#include <sstream>

#include "engine/math/vector2.h"
#include "engine/render/render.h"
#include "engine/time/game_loop.h"

namespace client {

Application::Application() = default;

int Application::Run() {
  engine::core::EngineRuntimeConfig config;
  config.window_config.title = "R-Type Client";
  config.window_config.size = engine::math::Vector2i(1280, 720);
  config.window_config.vsync = true;
  config.window_config.target_fps = 60;
  config.log_level = engine::util::LogLevel::kInfo;

  engine_ = engine::core::EngineRuntime::Create(config);
  if (!engine_) {
    return 1;
  }

  engine_->Logger().Info("Client runtime booted");

  engine::time::VariableTimestepLoop loop(
      static_cast<float>(config.window_config.target_fps));

  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });
  return 0;
}

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
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
