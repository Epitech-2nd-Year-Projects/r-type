#include "lobby_scene.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <utility>

#include "client_context.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

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

bool IsFourDigitPassword(const std::string& value) {
  if (value.size() != 4) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}
}  // namespace

LobbyScene::LobbyScene(ClientContext& context) : context_(context) {
  auto& renderer = context_.Renderer();
  renderer.LoadFont(std::string(constants::ui::kTitleFont),
                    std::string(constants::ui::kTitleFontPath));
  renderer.LoadFont(std::string(constants::ui::kBodyFont),
                    std::string(constants::ui::kBodyFontPath));
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  auto& runtime_config = context_.Config();
  lobby_host_ = runtime_config.GetString(
      std::string(constants::config::kClientHost), "127.0.0.1");
  const std::string port_value = runtime_config.GetString(
      std::string(constants::config::kClientPort), "4242");
  try {
    lobby_port_ = static_cast<std::uint16_t>(std::stoi(port_value));
  } catch (...) {
    lobby_port_ = 4242;
  }

  host_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  host_input_->SetPlaceholder("127.0.0.1");
  host_input_->SetText(lobby_host_);

  port_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  port_input_->SetPlaceholder("4242");
  port_input_->SetText(std::to_string(lobby_port_));

  name_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  name_input_->SetPlaceholder("Player name");
  name_input_->SetText(context_.Config().GetString(
      std::string(constants::config::kClientPlayerName), "Pilot"));

  refresh_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kRefreshButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Refresh", [this]() {
        std::string host = host_input_->GetText();
        std::string port_text = port_input_->GetText();
        if (host.empty()) host = "127.0.0.1";
        if (port_text.empty()) port_text = "4242";
        try {
          int port = std::stoi(port_text);
          if (port < 1 || port > 65535) {
            banner_text_ = "Port must be between 1 and 65535";
            banner_expiry_ = std::chrono::steady_clock::now() +
                             constants::ui::Lobby::kBannerDuration;
            return;
          }
          lobby_host_ = host;
          lobby_port_ = static_cast<std::uint16_t>(port);
          context_.RefreshRoomList(lobby_host_, lobby_port_);
        } catch (const std::exception& e) {
          banner_text_ = std::string("Invalid port: ") + e.what();
          banner_expiry_ = std::chrono::steady_clock::now() +
                           constants::ui::Lobby::kBannerDuration;
        }
      });

  create_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kCreateButtonSize,
                             constants::ui::Lobby::kCreateButtonSize},
      "+", [this]() { OpenCreateModal(); });

  ui_elements_.push_back(host_input_);
  ui_elements_.push_back(port_input_);
  ui_elements_.push_back(name_input_);
  ui_elements_.push_back(refresh_button_);
  ui_elements_.push_back(create_button_);

  const auto btn_tex = renderer.LoadTextureFromFile(
      std::string(constants::ui::kButtonTextureLargePath));
  const auto white = constants::ui::kButtonBaseColor;
  if (btn_tex) {
    const auto hover = constants::ui::kButtonHoverColor;
    const auto press = constants::ui::kButtonPressColor;
    refresh_button_->SetTexture(btn_tex);
    create_button_->SetTexture(btn_tex);
    refresh_button_->SetColors(white, hover, press);
    create_button_->SetColors(white, hover, press);
  }

  BuildModal();
  context_.RefreshRoomList(lobby_host_, lobby_port_);
}

void LobbyScene::BuildModal() {
  modal_room_name_input_ = std::make_shared<ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  modal_room_name_input_->SetPlaceholder("Room name");

  modal_max_players_input_ = std::make_shared<ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  modal_max_players_input_->SetPlaceholder("Max players (1-255)");
  modal_max_players_input_->SetText("4");

  modal_privacy_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Public", [this]() {
        modal_private_ = !modal_private_;
        modal_privacy_button_->SetText(modal_private_ ? "Private" : "Public");
        if (!modal_private_) {
          modal_password_input_->SetText("");
        }
      });

  modal_password_input_ = std::make_shared<ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  modal_password_input_->SetPlaceholder("4-digit password");

  modal_primary_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Create", [this]() {
        if (modal_mode_ == ModalMode::kCreate) {
          std::string name = modal_room_name_input_->GetText();
          std::string max_players_str = modal_max_players_input_->GetText();
          if (name.empty()) {
            banner_text_ = "Room name required";
            banner_expiry_ = std::chrono::steady_clock::now() +
                             constants::ui::Lobby::kBannerDuration;
            return;
          }
          if (max_players_str.empty()) {
            max_players_str = "4";
          }
          try {
            int max_players = std::stoi(max_players_str);
            max_players = std::clamp(max_players, 1, 255);
            std::string password_input = modal_password_input_->GetText();
            if (!password_input.empty() &&
                !IsFourDigitPassword(password_input)) {
              banner_text_ = "Password must be exactly 4 digits";
              banner_expiry_ = std::chrono::steady_clock::now() +
                               constants::ui::Lobby::kBannerDuration;
              return;
            }
            context_.CreateRoom(lobby_host_, lobby_port_, name, modal_private_,
                                std::move(password_input),
                                static_cast<std::uint16_t>(max_players));
            CloseModal();
          } catch (const std::exception& e) {
            banner_text_ = std::string("Invalid room parameters: ") + e.what();
            banner_expiry_ = std::chrono::steady_clock::now() +
                             constants::ui::Lobby::kBannerDuration;
          }
        } else {
          const std::string password = modal_password_input_->GetText();
          if (!IsFourDigitPassword(password)) {
            banner_text_ = "Enter a 4-digit password to join";
            banner_expiry_ = std::chrono::steady_clock::now() +
                             constants::ui::Lobby::kBannerDuration;
            return;
          }
          protocol::RoomSummary target{};
          target.room_code = pending_join_room_code_;
          target.room_name = pending_join_room_name_;
          target.is_private = true;
          JoinRoom(target, password);
          CloseModal();
        }
      });

  modal_cancel_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Cancel", [this]() { CloseModal(); });

  modal_elements_ = {modal_room_name_input_, modal_max_players_input_,
                     modal_privacy_button_,  modal_password_input_,
                     modal_primary_button_,  modal_cancel_button_};
  create_modal_elements_ = {modal_room_name_input_, modal_max_players_input_,
                            modal_privacy_button_, modal_primary_button_,
                            modal_cancel_button_};
  join_modal_elements_ = {modal_password_input_, modal_primary_button_,
                          modal_cancel_button_};
}

void LobbyScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();

  const bool is_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);
  if (is_down) {
    const auto pos = input.GetMousePosition();
    auto set_focus = [&](const std::shared_ptr<ui::TextInput>& field) {
      if (!field) return;
      engine::math::RectF rect{field->GetPosition(), field->GetSize()};
      field->SetFocused(rect.Contains(pos));
    };
    set_focus(host_input_);
    set_focus(port_input_);
    set_focus(name_input_);
    if (show_modal_) {
      if (modal_mode_ == ModalMode::kCreate) {
        set_focus(modal_room_name_input_);
        set_focus(modal_max_players_input_);
        if (modal_private_) {
          set_focus(modal_password_input_);
        }
      } else {
        set_focus(modal_password_input_);
      }
    }
  }

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
  for (auto& elem : room_buttons_) {
    elem->Update(dt, input);
  }
  if (show_modal_) {
    std::vector<std::shared_ptr<ui::UIElement>> active_modal =
        modal_mode_ == ModalMode::kCreate ? create_modal_elements_
                                          : join_modal_elements_;
    if (modal_mode_ == ModalMode::kCreate && modal_private_) {
      active_modal.push_back(modal_password_input_);
    }
    for (auto& elem : active_modal) {
      elem->Update(dt, input);
    }
  }

  RefreshRoomButtons();

  const auto now = std::chrono::steady_clock::now();
  if (!banner_text_.empty() && banner_expiry_ <= now) {
    banner_text_.clear();
  }

  if (input.IsKeyDown(engine::input::Key::kEscape)) {
    context_.OnQuitToMenu();
  }

  if (auto created = context_.ConsumeLastRoomCreation()) {
    std::string message = created->message;
    if (created->success && created->room) {
      message = "Room " + created->room->room_name + " created";
      if (created->room->is_private && !created->room_password.empty()) {
        message += " • Password: " + created->room_password;
      }
    }
    banner_text_ = message;
    banner_expiry_ = now + constants::ui::Lobby::kBannerDuration;
  }
}

void LobbyScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);

  const auto panel = constants::ui::Lobby::kPanelColor;
  const auto accent = constants::ui::Lobby::kAccentColor;
  const float margin = constants::ui::Lobby::kPanelMargin;

  const float list_top = constants::ui::Lobby::kListTop;
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  renderer.DrawText("Available rooms",
                    {margin, list_top - constants::ui::Lobby::kListTitleOffset},
                    constants::ui::Lobby::kListTitleFontSize,
                    engine::render::Color::White());
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  renderer.DrawText(
      context_.RoomDirectoryStatus(),
      {margin, list_top - constants::ui::Lobby::kListStatusOffset},
      constants::ui::Lobby::kListStatusFontSize,
      constants::ui::Lobby::kMutedTextColor);

  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  for (auto& elem : room_buttons_) {
    elem->Draw(renderer);
  }

  if (!banner_text_.empty()) {
    renderer.DrawText(banner_text_,
                      {constants::ui::Lobby::kBannerOffsetX,
                       height - constants::ui::Lobby::kBannerOffsetBottom},
                      constants::ui::Lobby::kBannerFontSize,
                      engine::render::Color::White());
  }

  if (show_modal_) {
    const engine::render::Color overlay = constants::ui::Lobby::kOverlayColor;
    renderer.DrawRect({0.0f, 0.0f, width, height}, overlay);
    const float modal_x = (width - constants::ui::Lobby::kModalWidth) * 0.5f;
    const float modal_y = (height - constants::ui::Lobby::kModalHeight) * 0.5f;
    renderer.DrawRect({modal_x, modal_y, constants::ui::Lobby::kModalWidth,
                       constants::ui::Lobby::kModalHeight},
                      panel);
    const std::string title =
        modal_mode_ == ModalMode::kCreate ? "Create a room" : "Enter password";
    renderer.SetFont(std::string(constants::ui::kTitleFont));
    renderer.DrawText(title,
                      {modal_x + constants::ui::Lobby::kModalPaddingX,
                       modal_y + constants::ui::Lobby::kModalTitleOffsetY},
                      constants::ui::Lobby::kModalTitleFontSize, accent);
    renderer.SetFont(std::string(constants::ui::kBodyFont));
    if (modal_mode_ == ModalMode::kCreate) {
      renderer.DrawText("Room name",
                        {modal_x + constants::ui::Lobby::kModalPaddingX,
                         modal_y + constants::ui::Lobby::kModalLabelRow1Y},
                        constants::ui::Lobby::kModalLabelFontSize,
                        engine::render::Color::White());
      renderer.DrawText("Max players",
                        {modal_x + constants::ui::Lobby::kModalPaddingX,
                         modal_y + constants::ui::Lobby::kModalLabelRow2Y},
                        constants::ui::Lobby::kModalLabelFontSize,
                        engine::render::Color::White());
      renderer.DrawText(modal_private_ ? "Private lobby" : "Public lobby",
                        {modal_x + constants::ui::Lobby::kModalLabelValueX,
                         modal_y + constants::ui::Lobby::kModalLabelRow2Y},
                        constants::ui::Lobby::kModalLabelFontSize,
                        constants::ui::Lobby::kSoftTextColor);
      if (modal_private_) {
        renderer.DrawText("Password (4 digits)",
                          {modal_x + constants::ui::Lobby::kModalPaddingX,
                           modal_y + constants::ui::Lobby::kModalPrivateLabelY},
                          constants::ui::Lobby::kModalLabelFontSize,
                          constants::ui::Lobby::kSoftTextColor);
      }
    } else {
      std::string subtitle = "Join " + pending_join_room_name_;
      renderer.DrawText(subtitle,
                        {modal_x + constants::ui::Lobby::kModalPaddingX,
                         modal_y + constants::ui::Lobby::kModalLabelRow1Y},
                        constants::ui::Lobby::kModalSubtitleFontSize,
                        constants::ui::Lobby::kSoftTextColor);
    }
    std::vector<std::shared_ptr<ui::UIElement>> active_modal =
        modal_mode_ == ModalMode::kCreate ? create_modal_elements_
                                          : join_modal_elements_;
    if (modal_mode_ == ModalMode::kCreate && modal_private_) {
      active_modal.push_back(modal_password_input_);
    }
    for (auto& elem : active_modal) {
      elem->Draw(renderer);
    }
  }
}

void LobbyScene::RefreshRoomButtons() {
  const auto rooms = context_.RoomDirectoryRooms();
  const auto hash = HashRooms(rooms);
  if (hash == last_rooms_hash_) {
    return;
  }
  last_rooms_hash_ = hash;
  room_buttons_.clear();
  const auto btn_tex = context_.Renderer().LoadTextureFromFile(
      std::string(constants::ui::kButtonTextureLargePath));
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
    auto button = std::make_shared<ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{0.0f, constants::ui::Lobby::kRoomButtonHeight},
        label, [this, room]() {
          if (room.is_private) {
            OpenJoinModal(room);
            return;
          }
          JoinRoom(room, "");
        });
    if (btn_tex) {
      button->SetTexture(btn_tex);
      button->SetColors(white, hover, press);
    }
    room_buttons_.push_back(button);
  }
}

void LobbyScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float margin = constants::ui::Lobby::kPanelMargin;

  const float controls_y = constants::ui::Lobby::kControlsY;
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  const float field_y = controls_y + (constants::ui::Lobby::kButtonHeight -
                                      constants::ui::Lobby::kFieldHeight) /
                                         2.0f;
  renderer.DrawText("Server address",
                    {margin, controls_y - constants::ui::Lobby::kLabelOffsetY},
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);
  renderer.DrawText("Server port",
                    {host_input_->GetPosition().x + host_input_->GetSize().x +
                         constants::ui::Lobby::kHostPortSpacing,
                     controls_y - constants::ui::Lobby::kLabelOffsetY},
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);
  renderer.DrawText("Player name",
                    {port_input_->GetPosition().x + port_input_->GetSize().x +
                         constants::ui::Lobby::kPortNameSpacing,
                     controls_y - constants::ui::Lobby::kLabelOffsetY},
                    constants::ui::Lobby::kModalLabelFontSize,
                    constants::ui::Lobby::kSoftTextColor);

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

  create_button_->SetPosition(
      {width - margin - create_button_->GetSize().x, controls_y});
  refresh_button_->SetPosition({create_button_->GetPosition().x -
                                    refresh_button_->GetSize().x -
                                    constants::ui::Lobby::kRefreshCreateSpacing,
                                controls_y});

  const float list_top = constants::ui::Lobby::kListTop;
  const float list_width = width - margin * 2.0f;
  const auto room_area_width = list_width;
  float room_y = list_top + constants::ui::Lobby::kRoomListTopPadding;
  for (auto& button : room_buttons_) {
    button->SetPosition({margin, room_y});
    button->SetSize({room_area_width, constants::ui::Lobby::kRoomButtonHeight});
    room_y += constants::ui::Lobby::kRoomButtonHeight +
              constants::ui::Lobby::kRoomListSpacing;
  }

  if (show_modal_) {
    ApplyModalLayout();
  }
}

void LobbyScene::OpenCreateModal() {
  modal_mode_ = ModalMode::kCreate;
  modal_private_ = false;
  modal_primary_button_->SetText("Create");
  modal_privacy_button_->SetText("Public");
  modal_room_name_input_->SetText("");
  modal_max_players_input_->SetText("4");
  modal_password_input_->SetPlaceholder("Private password (optional)");
  modal_password_input_->SetText("");
  show_modal_ = true;
}

void LobbyScene::OpenJoinModal(const protocol::RoomSummary& room) {
  modal_mode_ = ModalMode::kJoinPrivate;
  pending_join_room_code_ = room.room_code;
  pending_join_room_name_ = room.room_name;
  modal_primary_button_->SetText("Join");
  modal_password_input_->SetPlaceholder("Enter 4-digit password");
  modal_password_input_->SetText("");
  show_modal_ = true;
}

void LobbyScene::CloseModal() {
  show_modal_ = false;
  pending_join_room_code_.clear();
  pending_join_room_name_.clear();
}

void LobbyScene::JoinRoom(const protocol::RoomSummary& room,
                          const std::string& password) {
  std::string host = host_input_->GetText();
  std::string port_text = port_input_->GetText();
  std::string name = name_input_->GetText();
  if (host.empty()) {
    host = "127.0.0.1";
  }
  if (port_text.empty()) {
    port_text = "4242";
  }
  int port = 4242;
  try {
    port = std::stoi(port_text);
  } catch (...) {
    banner_text_ = "Invalid port";
    banner_expiry_ = std::chrono::steady_clock::now() +
                     constants::ui::Lobby::kBannerDuration;
    return;
  }
  if (port < 1 || port > 65535) {
    banner_text_ = "Port must be between 1 and 65535";
    banner_expiry_ = std::chrono::steady_clock::now() +
                     constants::ui::Lobby::kBannerDuration;
    return;
  }
  if (name.empty()) {
    name = "Pilot";
  }
  lobby_host_ = host;
  lobby_port_ = static_cast<std::uint16_t>(port);
  context_.SetConnectionConfig(lobby_host_, static_cast<int>(lobby_port_), name,
                               room.room_code, password);
  context_.StartConnection();
}

void LobbyScene::ApplyModalLayout() {
  const auto window_size = context_.Window().GetSize();
  const float modal_x =
      (static_cast<float>(window_size.x) - constants::ui::Lobby::kModalWidth) *
      0.5f;
  const float modal_y =
      (static_cast<float>(window_size.y) - constants::ui::Lobby::kModalHeight) *
      0.5f;

  if (modal_mode_ == ModalMode::kCreate) {
    modal_room_name_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalRoomNameInputY});
    modal_room_name_input_->SetSize(
        {constants::ui::Lobby::kModalWidth -
             constants::ui::Lobby::kModalPaddingX * 2.0f,
         constants::ui::Lobby::kFieldHeight});

    modal_max_players_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalMaxPlayersInputY});
    modal_max_players_input_->SetSize(
        {constants::ui::Lobby::kModalMaxPlayersWidth,
         constants::ui::Lobby::kFieldHeight});

    modal_privacy_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPrivacyButtonX,
         modal_max_players_input_->GetPosition().y -
             constants::ui::Lobby::kModalPrivacyButtonOffsetY});
    modal_privacy_button_->SetSize(
        {constants::ui::Lobby::kModalActionButtonWidth,
         constants::ui::Lobby::kButtonHeight});
    if (modal_private_) {
      modal_password_input_->SetPosition(
          {modal_x + constants::ui::Lobby::kModalPaddingX,
           modal_y + constants::ui::Lobby::kModalPasswordInputY});
      modal_password_input_->SetSize(
          {constants::ui::Lobby::kModalWidth -
               constants::ui::Lobby::kModalPaddingX * 2.0f,
           constants::ui::Lobby::kFieldHeight});
    }

    modal_primary_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPrimaryButtonY});
    modal_cancel_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalWidth -
             modal_cancel_button_->GetSize().x -
             constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPrimaryButtonY});
  } else if (modal_mode_ == ModalMode::kJoinPrivate) {
    modal_password_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPasswordJoinInputY});
    modal_password_input_->SetSize(
        {constants::ui::Lobby::kModalWidth -
             constants::ui::Lobby::kModalPaddingX * 2.0f,
         constants::ui::Lobby::kFieldHeight});

    modal_primary_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalJoinButtonY});
    modal_cancel_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalWidth -
             modal_cancel_button_->GetSize().x -
             constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalJoinButtonY});
  }

  modal_primary_button_->SetSize({constants::ui::Lobby::kModalActionButtonWidth,
                                  constants::ui::Lobby::kButtonHeight});
  modal_cancel_button_->SetSize({constants::ui::Lobby::kModalActionButtonWidth,
                                 constants::ui::Lobby::kButtonHeight});
}

bool LobbyScene::IsInputCaptured() const {
  if (host_input_ && host_input_->IsFocused()) return true;
  if (port_input_ && port_input_->IsFocused()) return true;
  if (name_input_ && name_input_->IsFocused()) return true;

  if (show_modal_) {
    if (modal_room_name_input_ && modal_room_name_input_->IsFocused())
      return true;
    if (modal_max_players_input_ && modal_max_players_input_->IsFocused())
      return true;
    if (modal_password_input_ && modal_password_input_->IsFocused())
      return true;
  }
  return false;
}

}  // namespace client
