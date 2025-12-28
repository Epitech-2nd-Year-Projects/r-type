#include "in_game_scene.h"

#include "client_context.h"
#include "constants/input_constants.h"
#include "engine/input.h"
#include "protocol/command.h"

namespace client {

InGameScene::InGameScene(ClientContext& context) : context_(context) {
  auto& input = context_.Input();
  pause_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
}

void InGameScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = context_.Input();

  const bool pause_pressed =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
  if (pause_pressed && !pause_pressed_) {
    context_.OnGamePause();
  }
  pause_pressed_ = pause_pressed;

  const bool toggle =
      input.IsActionActive(std::string(constants::input::kActionToggleReady));
  if (toggle && !toggle_pressed_) {
    is_ready_ = !is_ready_;
    protocol::CommandPayload payload;
    payload.command_id =
        static_cast<std::uint16_t>(is_ready_ ? protocol::CommandType::kSetReady
                                             : protocol::CommandType::kUnready);
    context_.EnqueueCommand(payload);
  }
  toggle_pressed_ = toggle;

  const auto local_player = context_.LocalPlayerId();
  hud_.UpdatePlayers(context_.World(), local_player);
  hud_.UpdateWave(context_.CurrentWave());

  const bool connected = context_.ConnectionActive();
  hud_.UpdateNetwork(context_.LatestLatencyMs(), connected,
                     std::string(context_.ConnectionStatus()));
}

void InGameScene::Draw(engine::render::Renderer2D& renderer) {
  hud_.Draw(renderer, context_.Window().GetSize());
}

}  // namespace client
