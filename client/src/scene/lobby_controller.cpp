#include "lobby_controller.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <utility>

#include "client_asset_manager.h"
#include "client_config.h"
#include "client_context.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/math/rect.h"

namespace client {

namespace {

bool IsFourDigitPassword(const std::string& value) {
  if (value.size() != 4) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

}  // namespace

LobbyController::LobbyController(ClientContext& context,
                                 std::function<void()> open_create_modal)
    : context_(context), open_create_modal_(std::move(open_create_modal)) {
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
  } else {
    lobby_host_ = defaults.host;
    lobby_port_ = defaults.port;
  }

  host_input_ = std::make_shared<engine::ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  host_input_->SetPlaceholder("127.0.0.1");
  host_input_->SetText(lobby_host_);

  port_input_ = std::make_shared<engine::ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  port_input_->SetPlaceholder("4242");
  port_input_->SetText(std::to_string(lobby_port_));

  name_input_ = std::make_shared<engine::ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  name_input_->SetPlaceholder("Player name");
  name_input_->SetText(validation.valid ? validation.player_name
                                        : defaults.player_name);

  refresh_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kRefreshButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Refresh", [this]() { RefreshRoomList(); });

  create_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kCreateButtonSize,
                             constants::ui::Lobby::kCreateButtonSize},
      "+", [this]() {
        if (open_create_modal_) {
          open_create_modal_();
        }
      });

  controls_ = {host_input_, port_input_, name_input_, refresh_button_,
               create_button_};

  RefreshRoomList();
}

void LobbyController::Update(engine::time::TimeDelta dt,
                             engine::input::InputManager& input) {
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
  const float margin = constants::ui::Lobby::kPanelMargin;
  const float controls_y = constants::ui::Lobby::kControlsY;
  const float field_y = controls_y + (constants::ui::Lobby::kButtonHeight -
                                      constants::ui::Lobby::kFieldHeight) /
                                         2.0f;

  host_input_->SetSize({constants::ui::Lobby::kHostFieldWidth,
                        constants::ui::Lobby::kFieldHeight});
  host_input_->SetPosition({margin, field_y});

  port_input_->SetSize({constants::ui::Lobby::kPortFieldWidth,
                        constants::ui::Lobby::kFieldHeight});
  port_input_->SetPosition({host_input_->GetPosition().x +
                                host_input_->GetSize().x +
                                constants::ui::Lobby::kHostPortSpacing,
                            field_y});

  name_input_->SetSize({constants::ui::Lobby::kNameFieldWidth,
                        constants::ui::Lobby::kFieldHeight});
  name_input_->SetPosition({port_input_->GetPosition().x +
                                port_input_->GetSize().x +
                                constants::ui::Lobby::kPortNameSpacing,
                            field_y});

  refresh_button_->SetSize({constants::ui::Lobby::kRefreshButtonWidth,
                            constants::ui::Lobby::kButtonHeight});
  create_button_->SetSize({constants::ui::Lobby::kCreateButtonSize,
                           constants::ui::Lobby::kCreateButtonSize});

  create_button_->SetPosition(
      {width - margin - create_button_->GetSize().x, controls_y});
  refresh_button_->SetPosition({create_button_->GetPosition().x -
                                    refresh_button_->GetSize().x -
                                    constants::ui::Lobby::kRefreshCreateSpacing,
                                controls_y});

  host_label_pos_ = {margin, controls_y - constants::ui::Lobby::kLabelOffsetY};
  port_label_pos_ = {host_input_->GetPosition().x + host_input_->GetSize().x +
                         constants::ui::Lobby::kHostPortSpacing,
                     controls_y - constants::ui::Lobby::kLabelOffsetY};
  name_label_pos_ = {port_input_->GetPosition().x + port_input_->GetSize().x +
                         constants::ui::Lobby::kPortNameSpacing,
                     controls_y - constants::ui::Lobby::kLabelOffsetY};

  banner_pos_ = {constants::ui::Lobby::kBannerOffsetX,
                 height - constants::ui::Lobby::kBannerOffsetBottom};
}

void LobbyController::Draw(engine::render::Renderer2D& renderer) const {
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  renderer.DrawText("Server address", host_label_pos_,
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);
  renderer.DrawText("Server port", port_label_pos_,
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);
  renderer.DrawText("Player name", name_label_pos_,
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);

  for (auto& elem : controls_) {
    elem->Draw(renderer);
  }

  if (!banner_text_.empty()) {
    renderer.DrawText(banner_text_, banner_pos_,
                      constants::ui::Lobby::kBannerFontSize,
                      engine::render::Color::White());
  }
}

void LobbyController::ApplyButtonStyle(ClientAssetManager& assets) {
  const auto btn_tex =
      assets.GetTexture(constants::ui::kButtonTextureLargePath);
  const auto white = constants::ui::kButtonBaseColor;
  if (btn_tex) {
    const auto hover = constants::ui::kButtonHoverColor;
    const auto press = constants::ui::kButtonPressColor;
    refresh_button_->SetTexture(btn_tex);
    create_button_->SetTexture(btn_tex);
    refresh_button_->SetColors(white, hover, press);
    create_button_->SetColors(white, hover, press);
  }
}

void LobbyController::HandleFocus(const engine::input::InputManager& input) {
  const auto pos = input.GetMousePosition();
  auto set_focus = [&](const std::shared_ptr<engine::ui::TextInput>& field) {
    if (!field) {
      return;
    }
    engine::math::RectF rect{field->GetPosition(), field->GetSize()};
    field->SetFocused(rect.Contains(pos));
  };
  set_focus(host_input_);
  set_focus(port_input_);
  set_focus(name_input_);
}

bool LobbyController::IsInputCaptured() const {
  if (host_input_ && host_input_->IsFocused()) {
    return true;
  }
  if (port_input_ && port_input_->IsFocused()) {
    return true;
  }
  if (name_input_ && name_input_->IsFocused()) {
    return true;
  }
  return false;
}

bool LobbyController::TryCreateRoom(const std::string& room_name,
                                    const std::string& max_players_text,
                                    bool is_private, std::string password) {
  if (room_name.empty()) {
    SetBanner("Room name required");
    return false;
  }

  std::string max_players_str = max_players_text;
  if (max_players_str.empty()) {
    max_players_str = "4";
  }
  int max_players = 0;
  try {
    max_players = std::stoi(max_players_str);
  } catch (const std::exception& e) {
    SetBanner(std::string("Invalid room parameters: ") + e.what());
    return false;
  }

  max_players = std::clamp(max_players, 1, 255);

  if (!password.empty() && !IsFourDigitPassword(password)) {
    SetBanner("Password must be exactly 4 digits");
    return false;
  }

  context_.CreateRoom(lobby_host_, lobby_port_, room_name, is_private,
                      std::move(password),
                      static_cast<std::uint16_t>(max_players));
  return true;
}

bool LobbyController::TryJoinRoom(const protocol::RoomSummary& room,
                                  const std::string& password) {
  if (room.is_private && !IsFourDigitPassword(password)) {
    SetBanner("Enter a 4-digit password to join");
    return false;
  }

  const auto validation = ValidateConnectionFields(
      host_input_->GetText(), port_input_->GetText(), name_input_->GetText());
  if (!validation.valid) {
    SetBanner(validation.message);
    return false;
  }

  lobby_host_ = validation.host;
  lobby_port_ = validation.port;
  context_.SetConnectionConfig(lobby_host_, static_cast<int>(lobby_port_),
                               validation.player_name, room.room_code,
                               password);
  context_.StartConnection();
  return true;
}

void LobbyController::RefreshRoomList() {
  const auto validation = ValidateConnectionFields(
      host_input_->GetText(), port_input_->GetText(), name_input_->GetText());
  if (!validation.valid) {
    SetBanner(validation.message);
    return;
  }

  lobby_host_ = validation.host;
  lobby_port_ = validation.port;
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
