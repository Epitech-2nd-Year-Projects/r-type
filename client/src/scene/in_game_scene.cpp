#include "in_game_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"

namespace client {

InGameScene::InGameScene(Application& app) : app_(app) {
  hud_.UpdateWaveAndLevel(1u, 1u);
}

void InGameScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Cancel")) {
    app_.OnGamePause();
  }

  const bool toggle = input.IsActionActive("ToggleReady");
  if (toggle && !toggle_pressed_) {
    is_ready_ = !is_ready_;
    protocol::CommandPayload payload;
    payload.command_id = static_cast<std::uint16_t>(
        is_ready_ ? protocol::CommandType::kSetReady
                  : protocol::CommandType::kUnready);
    app_.GetWorldUpdateReceiver().EnqueueCommand(payload);
  }
  toggle_pressed_ = toggle;

  const auto local_player = app_.GetJoinFlow().player_id();
  hud_.UpdatePlayers(app_.World(), local_player);

  const auto join_state = app_.GetJoinFlow().state();
  const bool connected =
      join_state == JoinState::kConnected && app_.GetTransport().running();
  hud_.UpdateNetwork(app_.LatestLatencyMs(), connected,
                     app_.GetJoinFlow().status());
}

void InGameScene::Draw(engine::render::Renderer2D& renderer) {
  hud_.Draw(renderer, app_.GetEngine().Window().GetSize());
}

}  // namespace client
