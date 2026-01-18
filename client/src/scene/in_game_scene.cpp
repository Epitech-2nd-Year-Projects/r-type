#include "in_game_scene.h"

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "lobby_chat_service.h"
#include "protocol/command.h"
#include "protocol/gameplay_ping.h"

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

  chat_view_.SyncMessages(context_.ChatService().messages());
  chat_view_.Update(dt, input);
  LayoutChat();

  const bool ping_input = input.IsKeyDown(engine::input::Key::kG);

  if (ping_input) {
    if (!ping_pressed_) {
      ping_pressed_ = true;
    }
    ping_wheel_.Update(input, context_.Window().GetSize());
  } else {
    if (ping_pressed_) {
      ping_pressed_ = false;
      if (ping_wheel_.IsActive()) {
        auto selection = ping_wheel_.CommitSelection();
        if (selection) {
          protocol::GameplayPingPayload payload;
          payload.sender_id = context_.LocalPlayerId().value_or(0);
          payload.type = *selection;

          auto mouse = input.GetMousePosition();
          payload.x = mouse.x;
          payload.y = mouse.y;

          context_.EnqueueGameplayPing(payload);

          received_pings_.push_back({payload, 5.0f});
        }
      }
    }
  }

  float delta = dt.as_seconds();
  for (auto it = received_pings_.begin(); it != received_pings_.end();) {
    it->second -= delta;
    if (it->second <= 0) {
      it = received_pings_.erase(it);
    } else {
      ++it;
    }
  }
}

void InGameScene::OnGameplayPing(const protocol::GameplayPingPayload& ping) {
  received_pings_.push_back({ping, 5.0f});
}

void InGameScene::Draw(engine::render::Renderer2D& renderer) {
  hud_.Draw(renderer, context_.Window().GetSize());

  for (const auto& [ping, time] : received_pings_) {
    std::string label = "Ping";
    engine::render::Color color =
        engine::render::Color::FromBytes(200, 200, 220);
    switch (ping.type) {
      case protocol::PingType::kAttack:
        label = "ATTACK";
        color = engine::render::Color::FromBytes(255, 80, 80);
        break;
      case protocol::PingType::kDefend:
        label = "DEFEND";
        color = engine::render::Color::FromBytes(80, 120, 255);
        break;
      case protocol::PingType::kDanger:
        label = "DANGER";
        color = engine::render::Color::FromBytes(255, 180, 50);
        break;
      case protocol::PingType::kOnMyWay:
        label = "OMW";
        color = engine::render::Color::FromBytes(80, 255, 120);
        break;
      case protocol::PingType::kGeneric:
        label = "HERE";
        color = engine::render::Color::FromBytes(200, 200, 220);
        break;
    }

    if (time < 1.0f) {
      color = color.WithAlpha(time);
    }

    renderer.DrawCircle({ping.x, ping.y}, 10.0f,
                        color.WithAlpha(color.a * 0.3f));
    renderer.DrawCircle({ping.x, ping.y}, 6.0f, color);
    renderer.DrawText(label, {ping.x + 12.0f, ping.y - 8.0f}, 16, color);
  }

  chat_view_.Draw(renderer);

  if (ping_pressed_) {
    ping_wheel_.Draw(renderer);
  }
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
