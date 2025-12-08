#include "disconnected_scene.h"

#include <utility>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"

namespace client {

DisconnectedScene::DisconnectedScene(Application& app, std::string reason)
    : app_(app), reason_(std::move(reason)) {}

void DisconnectedScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    app_.OnQuitToMenu();
  }
}

void DisconnectedScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText("Disconnected", {300.0f, 200.0f}, 48.0f,
                    engine::render::Color::FromBytes(255, 0, 0));
  renderer.DrawText(reason_, {300.0f, 250.0f}, 20.0f,
                    engine::render::Color::White());
  renderer.DrawText("Press ENTER to Main Menu", {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
