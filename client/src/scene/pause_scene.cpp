#include "pause_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"

namespace client {

PauseScene::PauseScene(Application& app) : app_(app) {}

void PauseScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm") || input.IsActionActive("Cancel")) {
    app_.OnGameResume();
  } else if (input.IsActionActive("Quit")) {
    app_.OnQuitToMenu();
  }
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText("Paused", {300.0f, 200.0f}, 48.0f,
                    engine::render::Color::White());
  renderer.DrawText("Press ENTER to Resume", {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
  renderer.DrawText("Press Q to Quit to Main Menu", {300.0f, 350.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
