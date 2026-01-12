#include "lobby_scene.h"

#include <utility>

#include "client_asset_manager.h"
#include "client_config.h"
#include "client_context.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/renderer2d.h"
#include "protocol/lobby.h"
#include "ui/menu_background.h"

namespace client {

LobbyScene::LobbyScene(ClientContext& context)
    : context_(context),
      controller_(context, [this]() { modal_.OpenCreate(); }),
      room_list_view_(context,
                      [this](const protocol::RoomSummary& room) {
                        HandleRoomSelected(room);
                      }),
      modal_(
          [this](const std::string& room_name,
                 const std::string& max_players_text, bool is_private,
                 std::string password) {
            return controller_.TryCreateRoom(room_name, max_players_text,
                                             is_private, std::move(password));
          },
          [this](const protocol::RoomSummary& room,
                 const std::string& password) {
            return controller_.TryJoinRoom(room, password);
          }) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  controller_.ApplyButtonStyle(assets);

  const ClientConfig defaults{};
  const auto refresh_ms = context_.Config().GetInt(
      std::string(constants::config::kClientRoomListRefreshMs),
      static_cast<int>(defaults.room_list_refresh_ms));
  const int interval_ms = refresh_ms > 0
                              ? refresh_ms
                              : static_cast<int>(defaults.room_list_refresh_ms);
  room_list_refresh_interval_ = engine::time::TimeDelta::from_milliseconds(
      static_cast<float>(interval_ms));
  room_list_refresh_elapsed_ = engine::time::TimeDelta::zero();
}

void LobbyScene::Update(engine::time::TimeDelta dt) {
  room_list_view_.RefreshRooms(context_.RoomDirectoryRooms(),
                               context_.Assets());
  LayoutUi();

  if (room_list_refresh_interval_ > engine::time::TimeDelta::zero()) {
    room_list_refresh_elapsed_ += dt;
    if (room_list_refresh_elapsed_ >= room_list_refresh_interval_) {
      controller_.RefreshRoomList();
      room_list_refresh_elapsed_ = engine::time::TimeDelta::zero();
    }
  }

  auto& input = context_.Input();
  const bool modal_open = modal_.IsOpen();
  if (input.IsMouseButtonDown(engine::input::MouseButton::kLeft)) {
    if (modal_open) {
      modal_.HandleFocus(input);
    } else {
      controller_.HandleFocus(input);
    }
  }

  if (modal_open) {
    context_.MenuBackground().Update(dt);
    modal_.Update(dt, input);
  } else {
    controller_.Update(dt, input);
    room_list_view_.Update(dt, input);
  }

  if (!IsInputCaptured() && input.IsKeyDown(engine::input::Key::kEscape)) {
    context_.OnQuitToMenu();
  }
}

void LobbyScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void LobbyScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  room_list_view_.DrawBackground();
}

void LobbyScene::DrawForeground(engine::render::Renderer2D& renderer) {
  LayoutUi();
  room_list_view_.DrawForeground(renderer, context_.RoomDirectoryStatus());
  controller_.Draw(renderer);
  modal_.Draw(renderer);
}

bool LobbyScene::IsInputCaptured() const {
  return controller_.IsInputCaptured() || modal_.IsInputCaptured();
}

void LobbyScene::LayoutUi() {
  const auto window_size = context_.Window().GetSize();
  const engine::math::Vector2f size{static_cast<float>(window_size.x),
                                    static_cast<float>(window_size.y)};
  controller_.Layout(size);
  room_list_view_.Layout(size);
  if (modal_.IsOpen()) {
    modal_.Layout(size);
  }
}

void LobbyScene::HandleRoomSelected(const protocol::RoomSummary& room) {
  if (room.is_private) {
    modal_.OpenJoin(room);
    return;
  }
  controller_.TryJoinRoom(room, "");
}

}  // namespace client


