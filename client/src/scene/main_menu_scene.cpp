#include "main_menu_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"

namespace client {

MainMenuScene::MainMenuScene(Application& app) : app_(app) {}

void MainMenuScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    app_.StartConnection();
  }
}

void MainMenuScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText("R-Type Client", {300.0f, 200.0f}, 48.0f,
                    engine::render::Color::White());
  renderer.DrawText("Press ENTER to Connect", {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
