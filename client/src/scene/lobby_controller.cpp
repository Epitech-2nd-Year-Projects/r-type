#include "lobby_controller.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <utility>

#include "client_asset_manager.h"
#include "client_config.h"
#include "client_context.h"
#include "constants/client_constants.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/render/color.h"

namespace client {

namespace {

ui::MenuPointerConfig LobbyPointerConfig() {
  return ui::MenuPointerConfig{
      constants::ui::kMenuPointerFramePrefix,
      constants::ui::kMenuPointerFrameExtension,
      constants::ui::OptionsMenu::kPointerFrameCount,
      constants::ui::OptionsMenu::kPointerFrameDuration,
      constants::ui::OptionsMenu::kPointerHeightFactor,
      constants::ui::Lobby::kPointerSpacing,
      constants::ui::OptionsMenu::kPointerScaleFactor};
}

bool IsFourDigitPassword(const std::string& value) {
  if (value.size() != constants::client::kLobbyPasswordLength) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

}  // namespace

LobbyController::LobbyController(ClientContext& context,
                                 std::function<void()> open_create_modal)
    : context_(context),
      open_create_modal_(std::move(open_create_modal)),
      menu_effects_(context, LobbyPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& runtime_config = context_.Config();
  const ClientConfig defaults{};
  const std::string host_value = runtime_config.GetString(
      std::string(constants::config::kClientHost), defaults.host);
  const std::string port_value =
      runtime_config.GetString(std::string(constants::config::kClientPort),
                               std::to_string(defaults.port));
  const std::string name_value = runtime_config.GetString(
      std::string(constants::config::kClientPlayerName), defaults.player_name);
  const auto validation =
      ValidateConnectionFields(host_value, port_value, name_value);
  if (validation.valid) {
    lobby_host_ = validation.host;
    lobby_port_ = validation.port;
    player_name_ = validation.player_name;
  } else {
    lobby_host_ = defaults.host;
    lobby_port_ = defaults.port;
    player_name_ = defaults.player_name;
  }

  create_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kCreateButtonWidth,
                             constants::ui::Lobby::kCreateButtonHeight},
      "Create new Room", menu_effects_.WrapClick([this]() {
        if (open_create_modal_) {
          open_create_modal_();
        }
      }));

  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kBackButtonWidth,
                             constants::ui::Lobby::kBackButtonHeight},
      "Back", menu_effects_.WrapClick([this]() { context_.OnQuitToMenu(); }));

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);
  create_button_->SetColors(transparent, transparent, transparent);
  create_button_->SetTextColor(white);
  create_button_->SetTextScale(constants::ui::Lobby::kBackButtonTextScale);
  back_button_->SetColors(transparent, transparent, transparent);
  back_button_->SetTextColor(white);
  back_button_->SetTextScale(constants::ui::Lobby::kBackButtonTextScale);

  controls_ = {create_button_, back_button_};
  pointer_buttons_ = {create_button_, back_button_};

  menu_effects_.Load();
  RefreshRoomList();
}

void LobbyController::Update(engine::time::TimeDelta dt,
                             engine::input::InputManager& input) {
  menu_effects_.Update(dt, input, pointer_buttons_);
  for (auto& elem : controls_) {
    elem->Update(dt, input);
  }

  const auto now = std::chrono::steady_clock::now();
  if (!banner_text_.empty() && banner_expiry_ <= now) {
    banner_text_.clear();
  }

  UpdateBannerFromCreation();
}

void LobbyController::Layout(const engine::math::Vector2f& window_size) {
  const float width = window_size.x;
  const float height = window_size.y;

  create_button_->SetSize({constants::ui::Lobby::kCreateButtonWidth,
                           constants::ui::Lobby::kCreateButtonHeight});
  back_button_->SetSize({constants::ui::Lobby::kBackButtonWidth,
                         constants::ui::Lobby::kBackButtonHeight});

  const float header_bottom = constants::ui::Lobby::kHeaderTopPadding +
                              constants::ui::Lobby::kHeaderTitleFontSize +
                              constants::ui::Lobby::kHeaderDecorationSpacing +
                              constants::ui::Lobby::kHeaderDecorationHeight;
  const float create_button_y =
      header_bottom + constants::ui::Lobby::kHeaderDecorationBottomSpacing +
      constants::ui::Lobby::kCreateButtonOffsetY;
  create_button_->SetPosition({constants::ui::Lobby::kRoomListLeftPadding +
                                   constants::ui::Lobby::kCreateButtonOffsetX,
                               create_button_y});
  back_button_->SetPosition(
      {(width - back_button_->GetSize().x) * 0.5f,
       height - constants::ui::Lobby::kBackButtonBottomPadding -
           back_button_->GetSize().y});
}

void LobbyController::Draw(engine::render::Renderer2D& renderer) const {
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  create_button_->Draw(renderer);
  back_button_->Draw(renderer);
  menu_effects_.DrawPointers(renderer, pointer_buttons_);
}

void LobbyController::ApplyButtonStyle(ClientAssetManager& assets) {
  static_cast<void>(assets);
  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);
  create_button_->SetColors(transparent, transparent, transparent);
  create_button_->SetTextColor(white);
  create_button_->SetTextScale(constants::ui::Lobby::kBackButtonTextScale);
}

void LobbyController::HandleFocus(const engine::input::InputManager& input) {
  static_cast<void>(input);
}

bool LobbyController::IsInputCaptured() const { return false; }

bool LobbyController::TryCreateRoom(const std::string& room_name,
                                    const std::string& max_players_text,
                                    bool is_private, std::string password,
                                    protocol::Difficulty difficulty) {
  if (room_name.empty()) {
    SetBanner("Room name required");
    return false;
  }

  const std::string max_players_str =
      max_players_text.empty()
          ? std::to_string(constants::client::kLobbyDefaultMaxPlayers)
          : max_players_text;
  int max_players = 0;
  try {
    max_players = std::stoi(max_players_str);
  } catch (const std::exception& e) {
    SetBanner(std::string("Invalid room parameters: ") + e.what());
    return false;
  }

  max_players = std::clamp(max_players, constants::client::kLobbyMaxPlayersMin,
                           constants::client::kLobbyMaxPlayersMax);

  if (!password.empty() && !IsFourDigitPassword(password)) {
    SetBanner("Password must be exactly " +
              std::to_string(constants::client::kLobbyPasswordLength) +
              " digits");
    return false;
  }

  context_.CreateRoom(lobby_host_, lobby_port_, room_name, is_private,
                      std::move(password),
                      static_cast<std::uint16_t>(max_players), difficulty);
  return true;
}

bool LobbyController::TryJoinRoom(const protocol::RoomSummary& room,
                                  const std::string& password) {
  if (room.is_private && !IsFourDigitPassword(password)) {
    SetBanner("Enter a " +
              std::to_string(constants::client::kLobbyPasswordLength) +
              "-digit password to join");
    return false;
  }
  const std::string& player_nickname = context_.Profile().nickname;
  context_.SetConnectionConfig(lobby_host_, static_cast<int>(lobby_port_),
                               player_nickname, room.room_code, password);
  context_.StartConnection();
  return true;
}

void LobbyController::RefreshRoomList() {
  context_.RefreshRoomList(lobby_host_, lobby_port_);
}

void LobbyController::UpdateBannerFromCreation() {
  if (auto created = context_.ConsumeLastRoomCreation()) {
    std::string message = created->message;
    if (created->success && created->room) {
      message = "Room " + created->room->room_name + " created";
      if (created->room->is_private && !created->room_password.empty()) {
        message += " - Password: " + created->room_password;
      }
    }
    banner_text_ = message;
    banner_expiry_ = std::chrono::steady_clock::now() +
                     constants::ui::Lobby::kBannerDuration;
  }
}

void LobbyController::SetBanner(std::string message) {
  banner_text_ = std::move(message);
  banner_expiry_ =
      std::chrono::steady_clock::now() + constants::ui::Lobby::kBannerDuration;
}

}  // namespace client
