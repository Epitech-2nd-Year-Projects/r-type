#include "in_game_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"

namespace client {

InGameScene::InGameScene(Application& app) : app_(app) {}

void InGameScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Cancel")) {
    app_.OnGamePause();
  }
}

void InGameScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText("In Game", {300.0f, 50.0f}, 32.0f,
                    engine::render::Color::White());

  if (const auto player_id = app_.GetJoinFlow().player_id()) {
    renderer.DrawText("Player ID: " + std::to_string(*player_id),
                      {300.0f, 100.0f}, 18.0f,
                      engine::render::Color::FromBytes(180, 220, 255));
  }
}

}  // namespace client
