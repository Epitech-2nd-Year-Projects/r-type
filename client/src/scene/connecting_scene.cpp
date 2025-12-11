#include "connecting_scene.h"

#include "application.h"
#include "engine/render/color.h"
#include "join_flow.h"

namespace client {

ConnectingScene::ConnectingScene(Application& app) : app_(app) {}

void ConnectingScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& join_flow = app_.GetJoinFlow();

  if (join_flow.state() == JoinState::kConnected) {
    app_.OnConnected();
  } else if (join_flow.state() == JoinState::kRefused) {
    app_.OnConnectionFailed(join_flow.status());
  }
}

void ConnectingScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.DrawText(app_.GetJoinFlow().status(), {300.0f, 300.0f}, 24.0f,
                    engine::render::Color::White());
}

}  // namespace client
