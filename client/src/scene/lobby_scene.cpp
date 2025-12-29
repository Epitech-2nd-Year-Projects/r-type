#include "lobby_scene.h"

#include <utility>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/renderer2d.h"
#include "protocol/lobby.h"

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
}

void LobbyScene::Update(engine::time::TimeDelta dt) {
  room_list_view_.RefreshRooms(context_.RoomDirectoryRooms(),
                               context_.Assets());
  LayoutUi();

  auto& input = context_.Input();
  if (input.IsMouseButtonDown(engine::input::MouseButton::kLeft)) {
    controller_.HandleFocus(input);
    if (modal_.IsOpen()) {
      modal_.HandleFocus(input);
    }
  }

  controller_.Update(dt, input);
  room_list_view_.Update(dt, input);
  modal_.Update(dt, input);

  if (!IsInputCaptured() && input.IsKeyDown(engine::input::Key::kEscape)) {
    context_.OnQuitToMenu();
  }
}

void LobbyScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi();
  room_list_view_.Draw(renderer, context_.RoomDirectoryStatus());
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
