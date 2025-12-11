#include "connecting_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "join_flow.h"

namespace client {

ConnectingScene::ConnectingScene(Application& app) : app_(app) {}

void ConnectingScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();
  if (input.IsActionActive("Cancel")) {
    app_.OnQuitToMenu();
  }
}

void ConnectingScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText(app_.GetJoinFlow().status(), {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
