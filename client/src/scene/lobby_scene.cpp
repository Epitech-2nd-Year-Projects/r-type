#include "lobby_scene.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <utility>

#include "application.h"
#include "engine/app/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {
constexpr float kFieldHeight = 52.0f;
constexpr float kButtonHeight = 56.0f;
constexpr float kRoomButtonHeight = 64.0f;
constexpr float kModalWidth = 560.0f;
constexpr float kModalHeight = 500.0f;
constexpr auto kBannerDuration = std::chrono::seconds(6);
constexpr std::size_t kGoldenHashRatio = 0x9e3779b9;
constexpr std::size_t kHashPrivateSalt = 0xabcddcba;
constexpr std::size_t kHashPublicSeed = 0x12344321;
constexpr const char* kTitleFont = "title_font";
constexpr const char* kBodyFont = "body_font";

std::size_t HashRooms(const std::vector<protocol::RoomSummary>& rooms) {
  std::size_t value = rooms.size();
  for (const auto& room : rooms) {
    std::size_t local = std::hash<std::string>{}(room.room_code);
    local ^= std::hash<std::string>{}(room.room_name) + kGoldenHashRatio +
             (local << 6) + (local >> 2);
    local ^=
        static_cast<std::size_t>(room.player_count + 31u * room.max_players);
    local ^= static_cast<std::size_t>(room.is_private ? kHashPrivateSalt
                                                      : kHashPublicSeed);
    local ^= static_cast<std::size_t>(room.started ? 0xfeedbabe : 0x0);
    value ^= local + kGoldenHashRatio + (value << 6) + (value >> 2);
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

LobbyScene::LobbyScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont(kTitleFont, "assets/fonts/trajanpro_bold.otf");
  renderer.LoadFont(kBodyFont, "assets/fonts/Perpetua-Regular.otf");
  renderer.SetFont(kBodyFont);

  auto& runtime_config = app_.GetEngine().Config();
  lobby_host_ = runtime_config.GetString("client.host", "127.0.0.1");
  const std::string port_value =
      runtime_config.GetString("client.port", "4242");
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
  name_input_->SetText(
      app.GetEngine().Config().GetString("client.player_name", "Pilot"));

  refresh_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{140.0f, kButtonHeight},
      "Refresh", [this]() {
        std::string host = host_input_->GetText();
        std::string port_text = port_input_->GetText();
        if (host.empty()) host = "127.0.0.1";
        if (port_text.empty()) port_text = "4242";
        try {
          int port = std::stoi(port_text);
          if (port < 1 || port > 65535) {
            banner_text_ = "Port must be between 1 and 65535";
            banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
            return;
          }
          lobby_host_ = host;
          lobby_port_ = static_cast<std::uint16_t>(port);
          app_.RefreshRoomList(lobby_host_, lobby_port_);
        } catch (const std::exception& e) {
          banner_text_ = std::string("Invalid port: ") + e.what();
          banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
        }
      });

  create_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{56.0f, 56.0f}, "+",
      [this]() { OpenCreateModal(); });

  ui_elements_.push_back(host_input_);
  ui_elements_.push_back(port_input_);
  ui_elements_.push_back(name_input_);
  ui_elements_.push_back(refresh_button_);
  ui_elements_.push_back(create_button_);

  const auto btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  const auto white = engine::render::Color::White();
  if (btn_tex) {
    const auto hover = engine::render::Color::FromBytes(220, 220, 220);
    const auto press = engine::render::Color::FromBytes(180, 180, 180);
    refresh_button_->SetTexture(btn_tex);
    create_button_->SetTexture(btn_tex);
    refresh_button_->SetColors(white, hover, press);
    create_button_->SetColors(white, hover, press);
  }

  BuildModal();
  app_.RefreshRoomList(lobby_host_, lobby_port_);
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
      engine::math::Vector2f{}, engine::math::Vector2f{180.0f, kButtonHeight},
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
      engine::math::Vector2f{}, engine::math::Vector2f{160.0f, kButtonHeight},
      "Create", [this]() {
        if (modal_mode_ == ModalMode::kCreate) {
          std::string name = modal_room_name_input_->GetText();
          std::string max_players_str = modal_max_players_input_->GetText();
          if (name.empty()) {
            banner_text_ = "Room name required";
            banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
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
              banner_expiry_ =
                  std::chrono::steady_clock::now() + kBannerDuration;
              return;
            }
            app_.CreateRoom(lobby_host_, lobby_port_, name, modal_private_,
                            std::move(password_input),
                            static_cast<std::uint16_t>(max_players));
            CloseModal();
          } catch (const std::exception& e) {
            banner_text_ = std::string("Invalid room parameters: ") + e.what();
            banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
          }
        } else {
          const std::string password = modal_password_input_->GetText();
          if (!IsFourDigitPassword(password)) {
            banner_text_ = "Enter a 4-digit password to join";
            banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
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
      engine::math::Vector2f{}, engine::math::Vector2f{160.0f, kButtonHeight},
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
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();

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
    app_.OnQuitToMenu();
  }

  if (auto created = app_.ConsumeLastRoomCreation()) {
    std::string message = created->message;
    if (created->success && created->room) {
      message = "Room " + created->room->room_name + " created";
      if (created->room->is_private && !created->room_password.empty()) {
        message += " • Password: " + created->room_password;
      }
    }
    banner_text_ = message;
    banner_expiry_ = now + kBannerDuration;
  }
}

void LobbyScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);
  renderer.SetFont(kBodyFont);
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);

  const auto panel = engine::render::Color::FromBytes(18, 24, 40, 230);
  const auto accent = engine::render::Color::FromBytes(140, 186, 255, 255);
  const float margin = 32.0f;

  const float list_top = 220.0f;
  renderer.SetFont(kTitleFont);
  renderer.DrawText("Available rooms", {margin, list_top - 44.0f}, 28.0f,
                    engine::render::Color::White());
  renderer.SetFont(kBodyFont);
  renderer.DrawText(app_.RoomDirectoryStatus(), {margin, list_top - 14.0f},
                    18.0f,
                    engine::render::Color::FromBytes(180, 190, 210, 255));

  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  for (auto& elem : room_buttons_) {
    elem->Draw(renderer);
  }

  if (!banner_text_.empty()) {
    renderer.DrawText(banner_text_, {52.0f, height - 50.0f}, 18.0f,
                      engine::render::Color::White());
  }

  if (show_modal_) {
    const engine::render::Color overlay =
        engine::render::Color::FromBytes(0, 0, 0, 180);
    renderer.DrawRect({0.0f, 0.0f, width, height}, overlay);
    const float modal_x = (width - kModalWidth) * 0.5f;
    const float modal_y = (height - kModalHeight) * 0.5f;
    renderer.DrawRect({modal_x, modal_y, kModalWidth, kModalHeight}, panel);
    const std::string title =
        modal_mode_ == ModalMode::kCreate ? "Create a room" : "Enter password";
    renderer.SetFont(kTitleFont);
    renderer.DrawText(title, {modal_x + 24.0f, modal_y + 34.0f}, 28.0f, accent);
    renderer.SetFont(kBodyFont);
    if (modal_mode_ == ModalMode::kCreate) {
      renderer.DrawText("Room name", {modal_x + 24.0f, modal_y + 84.0f}, 16.0f,
                        engine::render::Color::White());
      renderer.DrawText("Max players", {modal_x + 24.0f, modal_y + 180.0f},
                        16.0f, engine::render::Color::White());
      renderer.DrawText(modal_private_ ? "Private lobby" : "Public lobby",
                        {modal_x + 240.0f, modal_y + 180.0f}, 16.0f,
                        engine::render::Color::FromBytes(200, 210, 230, 255));
      if (modal_private_) {
        renderer.DrawText("Password (4 digits)",
                          {modal_x + 24.0f, modal_y + 272.0f}, 16.0f,
                          engine::render::Color::FromBytes(200, 210, 230, 255));
      }
    } else {
      std::string subtitle = "Join " + pending_join_room_name_;
      renderer.DrawText(subtitle, {modal_x + 24.0f, modal_y + 84.0f}, 18.0f,
                        engine::render::Color::FromBytes(200, 210, 230, 255));
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
  const auto rooms = app_.RoomDirectoryRooms();
  const auto hash = HashRooms(rooms);
  if (hash == last_rooms_hash_) {
    return;
  }
  last_rooms_hash_ = hash;
  room_buttons_.clear();
  const auto btn_tex = app_.GetEngine().Renderer().LoadTextureFromFile(
      "assets/ui/button_large.png");
  const auto white = engine::render::Color::White();
  const auto hover = engine::render::Color::FromBytes(220, 220, 220);
  const auto press = engine::render::Color::FromBytes(180, 180, 180);

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
        engine::math::Vector2f{0.0f, kRoomButtonHeight}, label, [this, room]() {
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
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float margin = 32.0f;

  const float controls_y = 110.0f;
  renderer.SetFont(kBodyFont);
  const float field_y = controls_y + (kButtonHeight - kFieldHeight) / 2.0f;
  renderer.DrawText("Server address", {margin, controls_y - 20.0f}, 16.0f,
                    engine::render::Color::FromBytes(200, 210, 230, 255));
  renderer.DrawText(
      "Server port",
      {host_input_->GetPosition().x + host_input_->GetSize().x + 10.0f,
       controls_y - 20.0f},
      16.0f, engine::render::Color::FromBytes(200, 210, 230, 255));
  renderer.DrawText(
      "Player name",
      {port_input_->GetPosition().x + port_input_->GetSize().x + 20.0f,
       controls_y - 20.0f},
      16.0f, engine::render::Color::FromBytes(200, 210, 230, 255));

  host_input_->SetSize({260.0f, kFieldHeight});
  host_input_->SetPosition({margin, field_y});

  port_input_->SetSize({100.0f, kFieldHeight});
  port_input_->SetPosition(
      {host_input_->GetPosition().x + host_input_->GetSize().x + 10.0f,
       field_y});

  name_input_->SetSize({260.0f, kFieldHeight});
  name_input_->SetPosition(
      {port_input_->GetPosition().x + port_input_->GetSize().x + 20.0f,
       field_y});

  create_button_->SetPosition(
      {width - margin - create_button_->GetSize().x, controls_y});
  refresh_button_->SetPosition(
      {create_button_->GetPosition().x - refresh_button_->GetSize().x - 12.0f,
       controls_y});

  const float list_top = 220.0f;
  const float list_width = width - margin * 2.0f;
  const auto room_area_width = list_width;
  float room_y = list_top + 20.0f;
  for (auto& button : room_buttons_) {
    button->SetPosition({margin, room_y});
    button->SetSize({room_area_width, kRoomButtonHeight});
    room_y += kRoomButtonHeight + 10.0f;
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
    banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
    return;
  }
  if (port < 1 || port > 65535) {
    banner_text_ = "Port must be between 1 and 65535";
    banner_expiry_ = std::chrono::steady_clock::now() + kBannerDuration;
    return;
  }
  if (name.empty()) {
    name = "Pilot";
  }
  lobby_host_ = host;
  lobby_port_ = static_cast<std::uint16_t>(port);
  app_.SetConnectionConfig(lobby_host_, static_cast<int>(lobby_port_), name,
                           room.room_code, password);
  app_.StartConnection();
}

void LobbyScene::ApplyModalLayout() {
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float modal_x =
      (static_cast<float>(window_size.x) - kModalWidth) * 0.5f;
  const float modal_y =
      (static_cast<float>(window_size.y) - kModalHeight) * 0.5f;

  if (modal_mode_ == ModalMode::kCreate) {
    modal_room_name_input_->SetPosition({modal_x + 24.0f, modal_y + 102.0f});
    modal_room_name_input_->SetSize({kModalWidth - 48.0f, kFieldHeight});

    modal_max_players_input_->SetPosition({modal_x + 24.0f, modal_y + 198.0f});
    modal_max_players_input_->SetSize({180.0f, kFieldHeight});

    modal_privacy_button_->SetPosition(
        {modal_x + 220.0f, modal_max_players_input_->GetPosition().y - 4.0f});
    modal_privacy_button_->SetSize({160.0f, kButtonHeight});
    if (modal_private_) {
      modal_password_input_->SetPosition({modal_x + 24.0f, modal_y + 292.0f});
      modal_password_input_->SetSize({kModalWidth - 48.0f, kFieldHeight});
    }

    modal_primary_button_->SetPosition({modal_x + 24.0f, modal_y + 420.0f});
    modal_cancel_button_->SetPosition(
        {modal_x + kModalWidth - modal_cancel_button_->GetSize().x - 24.0f,
         modal_y + 420.0f});
  } else if (modal_mode_ == ModalMode::kJoinPrivate) {
    modal_password_input_->SetPosition({modal_x + 24.0f, modal_y + 128.0f});
    modal_password_input_->SetSize({kModalWidth - 48.0f, kFieldHeight});

    modal_primary_button_->SetPosition({modal_x + 24.0f, modal_y + 220.0f});
    modal_cancel_button_->SetPosition(
        {modal_x + kModalWidth - modal_cancel_button_->GetSize().x - 24.0f,
         modal_y + 220.0f});
  }

  modal_primary_button_->SetSize({160.0f, kButtonHeight});
  modal_cancel_button_->SetSize({160.0f, kButtonHeight});
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
