#include "in_game_scene.h"

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "lobby_chat_service.h"
#include "protocol/command.h"

namespace client {

namespace {
namespace ui_config = constants::ui;
}  // namespace

InGameScene::InGameScene(ClientContext& context)
    : context_(context), chat_view_(context, [this](std::string_view message) {
        std::string full_message;
        full_message.push_back(
            static_cast<char>(context_.Profile().chat_color_index));
        full_message.append(message);
        return context_.ChatService().SendMessage(full_message);
      }) {
  auto& input = context_.Input();
  pause_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));

  chat_view_.ApplyStyle(context_.Assets());
}

void InGameScene::Update(engine::time::TimeDelta dt) {
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

  // Update chat view - sync messages and handle input
  chat_view_.SyncMessages(context_.ChatService().messages());
  chat_view_.Update(dt, input);
  LayoutChat();
}

void InGameScene::Draw(engine::render::Renderer2D& renderer) {
  hud_.Draw(renderer, context_.Window().GetSize());
  chat_view_.Draw(renderer);
}

bool InGameScene::IsInputCaptured() const {
  return chat_view_.IsInputCaptured();
}

void InGameScene::LayoutChat() {
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);

  const float chat_width = ui_config::Lobby::kChatPanelWidth;
  const float chat_padding = ui_config::Lobby::kPanelMargin;
  const float chat_height = height * 0.4f;

  const engine::math::RectF chat_rect{width - chat_width - chat_padding,
                                      height - chat_height - chat_padding,
                                      chat_width, chat_height};
  chat_view_.SetDefaultBounds(chat_rect);
  chat_view_.Layout();
}

}  // namespace client
