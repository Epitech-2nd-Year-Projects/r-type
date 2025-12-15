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
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {
constexpr float kFieldHeight = 52.0f;
constexpr float kButtonHeight = 56.0f;
constexpr float kRoomButtonHeight = 64.0f;
constexpr float kModalWidth = 560.0f;
constexpr float kModalHeight = 360.0f;
constexpr auto kBannerDuration = std::chrono::seconds(6);

std::size_t HashRooms(const std::vector<protocol::RoomSummary>& rooms) {
  std::size_t value = rooms.size();
  for (const auto& room : rooms) {
    std::size_t local = std::hash<std::string>{}(room.room_code);
    local ^= std::hash<std::string>{}(room.room_name) + 0x9e3779b9 +
             (local << 6) + (local >> 2);
    local ^=
        static_cast<std::size_t>(room.player_count + 31u * room.max_players);
    local ^=
        static_cast<std::size_t>(room.is_private ? 0xabcddcba : 0x12344321);
    value ^= local + 0x9e3779b9 + (value << 6) + (value >> 2);
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
  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  auto& runtime_config = app_.GetEngine().Config();
  lobby_host_ = runtime_config.GetString("client.host", "127.0.0.1");
  const std::string port_value =
      runtime_config.GetString("client.port", "4242");
  try {
    lobby_port_ = static_cast<std::uint16_t>(std::stoi(port_value));
  } catch (...) {
    lobby_port_ = 4242;
  }

  name_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  name_input_->SetPlaceholder("Crewmate name");
  name_input_->SetText(
      app.GetEngine().Config().GetString("client.player_name", "Pilot"));

  refresh_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{140.0f, kButtonHeight},
      "Refresh", [this]() { app_.RefreshRoomList(lobby_host_, lobby_port_); });

  back_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{120.0f, kButtonHeight},
      "Back", [this]() { app_.OnQuitToMenu(); });

  create_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{56.0f, 56.0f}, "+",
      [this]() { OpenCreateModal(); });

  ui_elements_.push_back(name_input_);
  ui_elements_.push_back(refresh_button_);
  ui_elements_.push_back(back_button_);
  ui_elements_.push_back(create_button_);

  const auto btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  const auto white = engine::render::Color::White();
  if (btn_tex) {
    const auto hover = engine::render::Color::FromBytes(220, 220, 220);
    const auto press = engine::render::Color::FromBytes(180, 180, 180);
    refresh_button_->SetTexture(btn_tex);
    back_button_->SetTexture(btn_tex);
    create_button_->SetTexture(btn_tex);
    refresh_button_->SetColors(white, hover, press);
    back_button_->SetColors(white, hover, press);
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
            app_.CreateRoom(lobby_host_, lobby_port_, name, modal_private_,
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
    set_focus(name_input_);
    if (show_modal_) {
      if (modal_mode_ == ModalMode::kCreate) {
        set_focus(modal_room_name_input_);
        set_focus(modal_max_players_input_);
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
    const auto& active_modal = modal_mode_ == ModalMode::kCreate
                                   ? create_modal_elements_
                                   : join_modal_elements_;
    for (auto& elem : active_modal) {
      elem->Update(dt, input);
    }
  }

  RefreshRoomButtons();

  const auto now = std::chrono::steady_clock::now();
  if (!banner_text_.empty() && banner_expiry_ <= now) {
    banner_text_.clear();
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
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);

  const auto panel = engine::render::Color::FromBytes(18, 24, 40, 230);
  const auto accent = engine::render::Color::FromBytes(140, 186, 255, 255);

  renderer.DrawText("Ready for launch", {40.0f, 60.0f}, 42.0f, accent);
  renderer.DrawText("Available rooms", {40.0f, 108.0f}, 24.0f,
                    engine::render::Color::White());
  renderer.DrawText(app_.RoomDirectoryStatus(), {40.0f, 136.0f}, 18.0f,
                    engine::render::Color::FromBytes(180, 190, 210, 255));

  renderer.DrawText("Crewmate", {40.0f, 174.0f}, 16.0f,
                    engine::render::Color::FromBytes(200, 210, 230, 255));
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
    renderer.DrawText(title, {modal_x + 24.0f, modal_y + 34.0f}, 28.0f, accent);
    if (modal_mode_ == ModalMode::kCreate) {
      renderer.DrawText("Room name", {modal_x + 24.0f, modal_y + 84.0f}, 16.0f,
                        engine::render::Color::White());
      renderer.DrawText("Max players", {modal_x + 24.0f, modal_y + 170.0f},
                        16.0f, engine::render::Color::White());
      renderer.DrawText(modal_private_ ? "Private lobby" : "Public lobby",
                        {modal_x + 240.0f, modal_y + 170.0f}, 16.0f,
                        engine::render::Color::FromBytes(200, 210, 230, 255));
    } else {
      std::string subtitle = "Join " + pending_join_room_name_;
      renderer.DrawText(subtitle, {modal_x + 24.0f, modal_y + 84.0f}, 18.0f,
                        engine::render::Color::FromBytes(200, 210, 230, 255));
    }
    const auto& active_modal = modal_mode_ == ModalMode::kCreate
                                   ? create_modal_elements_
                                   : join_modal_elements_;
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
    const std::string label = room.room_name + "   [" +
                              (room.is_private ? "Private" : "Public") + "]  " +
                              std::to_string(room.player_count) + "/" +
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
  (void)renderer;
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float margin = 56.0f;
  const float list_top = 220.0f;
  const float list_width = width - margin * 2.0f;

  name_input_->SetSize({260.0f, kFieldHeight});
  name_input_->SetPosition({margin, 190.0f});

  back_button_->SetPosition(
      {width - margin - back_button_->GetSize().x, 42.0f});
  refresh_button_->SetPosition(
      {width - margin - refresh_button_->GetSize().x, 190.0f});
  create_button_->SetPosition(
      {refresh_button_->GetPosition().x - create_button_->GetSize().x - 12.0f,
       190.0f});

  const auto room_area_width = list_width - 12.0f;
  float room_y = list_top;
  for (auto& button : room_buttons_) {
    button->SetPosition({margin + 6.0f, room_y});
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
  modal_password_input_->SetText("");
  show_modal_ = true;
}

void LobbyScene::OpenJoinModal(const protocol::RoomSummary& room) {
  modal_mode_ = ModalMode::kJoinPrivate;
  pending_join_room_code_ = room.room_code;
  pending_join_room_name_ = room.room_name;
  modal_primary_button_->SetText("Join");
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
  std::string name = name_input_->GetText();
  if (name.empty()) {
    name = "Pilot";
  }
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

    modal_max_players_input_->SetPosition({modal_x + 24.0f, modal_y + 188.0f});
    modal_max_players_input_->SetSize({180.0f, kFieldHeight});

    modal_privacy_button_->SetPosition(
        {modal_x + 220.0f, modal_max_players_input_->GetPosition().y - 4.0f});
    modal_privacy_button_->SetSize({160.0f, kButtonHeight});
  }

  modal_primary_button_->SetPosition({modal_x + 24.0f, modal_y + 280.0f});
  modal_primary_button_->SetSize({160.0f, kButtonHeight});
  modal_cancel_button_->SetPosition(
      {modal_x + kModalWidth - modal_cancel_button_->GetSize().x - 24.0f,
       modal_y + 280.0f});
  modal_cancel_button_->SetSize({160.0f, kButtonHeight});

  if (modal_mode_ == ModalMode::kJoinPrivate) {
    modal_password_input_->SetPosition({modal_x + 24.0f, modal_y + 128.0f});
    modal_password_input_->SetSize({kModalWidth - 48.0f, kFieldHeight});
  }
}

}  // namespace client
