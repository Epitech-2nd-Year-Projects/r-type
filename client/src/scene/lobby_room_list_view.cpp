#include "lobby_room_list_view.h"

#include <functional>
#include <string>
#include <utility>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "ui/menu_background.h"

namespace client {

namespace {

std::size_t HashRooms(const std::vector<protocol::RoomSummary>& rooms) {
  std::size_t value = rooms.size();
  for (const auto& room : rooms) {
    std::size_t local = std::hash<std::string>{}(room.room_code);
    local ^= std::hash<std::string>{}(room.room_name) +
             constants::ui::Lobby::kGoldenHashRatio + (local << 6) +
             (local >> 2);
    local ^=
        static_cast<std::size_t>(room.player_count + 31u * room.max_players);
    local ^= static_cast<std::size_t>(
        room.is_private ? constants::ui::Lobby::kHashPrivateSalt
                        : constants::ui::Lobby::kHashPublicSeed);
    local ^= static_cast<std::size_t>(room.started ? 0xfeedbabe : 0x0);
    value ^= local + constants::ui::Lobby::kGoldenHashRatio + (value << 6) +
             (value >> 2);
  }
  return value;
}

}  // namespace

LobbyRoomListView::LobbyRoomListView(
    ClientContext& context,
    std::function<void(const protocol::RoomSummary&)> on_room_selected)
    : context_(context), on_room_selected_(std::move(on_room_selected)) {}

void LobbyRoomListView::Update(engine::time::TimeDelta dt,
                               engine::input::InputManager& input) {
  context_.MenuBackground().Update(dt);
  for (auto& button : room_buttons_) {
    button->Update(dt, input);
  }
}

void LobbyRoomListView::Layout(const engine::math::Vector2f& window_size) {
  const float width = window_size.x;
  const float margin = constants::ui::Lobby::kPanelMargin;
  const float list_top = constants::ui::Lobby::kListTop;

  title_pos_ = {margin, list_top - constants::ui::Lobby::kListTitleOffset};
  status_pos_ = {margin, list_top - constants::ui::Lobby::kListStatusOffset};

  const float list_width = width - margin * 2.0f;
  float room_y = list_top + constants::ui::Lobby::kRoomListTopPadding;
  for (auto& button : room_buttons_) {
    button->SetPosition({margin, room_y});
    button->SetSize({list_width, constants::ui::Lobby::kRoomButtonHeight});
    room_y += constants::ui::Lobby::kRoomButtonHeight +
              constants::ui::Lobby::kRoomListSpacing;
  }
}

void LobbyRoomListView::Draw(engine::render::Renderer2D& renderer,
                             std::string_view status_text) const {
  context_.MenuBackground().Draw(context_.Window());
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  renderer.DrawText("Available rooms", title_pos_,
                    constants::ui::Lobby::kListTitleFontSize,
                    engine::render::Color::White());

  renderer.SetFont(std::string(constants::ui::kBodyFont));
  renderer.DrawText(status_text, status_pos_,
                    constants::ui::Lobby::kListStatusFontSize,
                    constants::ui::Lobby::kMutedTextColor);

  for (auto& button : room_buttons_) {
    button->Draw(renderer);
  }
}

void LobbyRoomListView::RefreshRooms(
    const std::vector<protocol::RoomSummary>& rooms,
    ClientAssetManager& assets) {
  const auto hash = HashRooms(rooms);
  if (hash == last_rooms_hash_) {
    return;
  }
  last_rooms_hash_ = hash;
  room_buttons_.clear();

  if (!button_texture_) {
    button_texture_ = assets.GetTexture(constants::ui::kButtonTextureLargePath);
  }
  const auto white = constants::ui::kButtonBaseColor;
  const auto hover = constants::ui::kButtonHoverColor;
  const auto press = constants::ui::kButtonPressColor;

  for (const auto& room : rooms) {
    std::string label =
        room.room_name + "   [" + (room.is_private ? "Private" : "Public");
    if (room.started) {
      label += " | Started";
    }
    label += "]  " + std::to_string(room.player_count) + "/" +
             std::to_string(room.max_players);

    auto button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{0.0f, constants::ui::Lobby::kRoomButtonHeight},
        label, [this, room]() {
          if (on_room_selected_) {
            on_room_selected_(room);
          }
        });
    if (button_texture_) {
      button->SetTexture(button_texture_);
      button->SetColors(white, hover, press);
    }
    room_buttons_.push_back(button);
  }
}

}  // namespace client
