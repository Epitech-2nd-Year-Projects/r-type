#include "game_over_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"

namespace client {

GameOverScene::GameOverScene(Application& app) : app_(app) {}

void GameOverScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    app_.OnQuitToMenu();
  }
}

void GameOverScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText("Game Over", {300.0f, 200.0f}, 48.0f,
                    engine::render::Color::FromBytes(255, 0, 0));
  renderer.DrawText("Press ENTER to Main Menu", {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
