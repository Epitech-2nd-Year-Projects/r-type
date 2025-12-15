#include "lobby_scene.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {
constexpr float kFieldHeight = 48.0f;
constexpr float kButtonHeight = 52.0f;
constexpr float kRoomButtonHeight = 46.0f;

std::size_t HashRooms(const std::vector<protocol::RoomSummary>& rooms) {
  std::size_t value = rooms.size();
  for (const auto& room : rooms) {
    std::size_t local = std::hash<std::string>{}(room.room_code);
    local ^=
        static_cast<std::size_t>(room.player_count + 31u * room.max_players);
    local ^=
        static_cast<std::size_t>(room.is_private ? 0xabcddcba : 0x12344321);
    value ^= local + 0x9e3779b9 + (value << 6) + (value >> 2);
  }
  return value;
}

}  // namespace

LobbyScene::LobbyScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  host_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  host_input_->SetPlaceholder("127.0.0.1");
  host_input_->SetText(
      app.GetEngine().Config().GetString("client.host", "127.0.0.1"));

  port_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  port_input_->SetPlaceholder("4242");
  port_input_->SetText(
      app.GetEngine().Config().GetString("client.port", "4242"));

  name_input_ = std::make_shared<ui::TextInput>(engine::math::Vector2f{},
                                                engine::math::Vector2f{});
  name_input_->SetPlaceholder("Pilot");
  name_input_->SetText(
      app.GetEngine().Config().GetString("client.player_name", "Pilot"));

  refresh_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{160.0f, kButtonHeight},
      "Refresh", [this]() {
        std::string host = host_input_->GetText();
        std::string port_str = port_input_->GetText();
        if (host.empty()) host = "127.0.0.1";
        if (port_str.empty()) port_str = "4242";
        try {
          const int port = std::stoi(port_str);
          if (port < 1 || port > 65535) {
            LogLifecycle(engine::util::LogLevel::kWarn,
                         "Port must be between 1 and 65535");
            return;
          }
          app_.RefreshRoomList(host, static_cast<std::uint16_t>(port));
        } catch (const std::exception& e) {
          LogLifecycle(engine::util::LogLevel::kWarn,
                       std::string("Invalid port: ") + e.what());
        }
      });

  back_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{120.0f, kButtonHeight},
      "Back", [this]() { app_.OnQuitToMenu(); });

  create_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{52.0f, 52.0f}, "+",
      [this]() { OpenCreateModal(); });

  ui_elements_.push_back(host_input_);
  ui_elements_.push_back(port_input_);
  ui_elements_.push_back(name_input_);
  ui_elements_.push_back(refresh_button_);
  ui_elements_.push_back(back_button_);
  ui_elements_.push_back(create_button_);

  const auto btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  const auto white = engine::render::Color::White();
  if (btn_tex) {
    refresh_button_->SetTexture(btn_tex);
    back_button_->SetTexture(btn_tex);
    create_button_->SetTexture(btn_tex);
    const auto hover = engine::render::Color::FromBytes(220, 220, 220);
    const auto press = engine::render::Color::FromBytes(180, 180, 180);
    refresh_button_->SetColors(white, hover, press);
    back_button_->SetColors(white, hover, press);
    create_button_->SetColors(white, hover, press);
  }

  BuildModal();

  try {
    const int port = std::stoi(port_input_->GetText());
    app_.RefreshRoomList(host_input_->GetText(),
                         static_cast<std::uint16_t>(port));
  } catch (...) {
    // ignore bootstrap errors, handled by UI interactions.
  }
}

void LobbyScene::BuildModal() {
  modal_room_code_input_ = std::make_shared<ui::TextInput>(
      engine::math::Vector2f{}, engine::math::Vector2f{});
  modal_room_code_input_->SetPlaceholder("Room code (optional)");

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

  modal_create_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{140.0f, kButtonHeight},
      "Create", [this]() {
        std::string host = host_input_->GetText();
        std::string port_str = port_input_->GetText();
        std::string room_code = modal_room_code_input_->GetText();
        std::string max_players_str = modal_max_players_input_->GetText();
        if (host.empty()) host = "127.0.0.1";
        if (port_str.empty()) port_str = "4242";
        if (max_players_str.empty()) max_players_str = "4";
        try {
          const int port = std::stoi(port_str);
          if (port < 1 || port > 65535) {
            LogLifecycle(engine::util::LogLevel::kWarn,
                         "Port must be between 1 and 65535");
            return;
          }
          int max_players = std::stoi(max_players_str);
          max_players = std::clamp(max_players, 1, 255);
          app_.CreateRoom(host, static_cast<std::uint16_t>(port), room_code,
                          modal_private_,
                          static_cast<std::uint16_t>(max_players));
          CloseCreateModal();
        } catch (const std::exception& e) {
          LogLifecycle(engine::util::LogLevel::kWarn,
                       std::string("Invalid room parameters: ") + e.what());
        }
      });

  modal_cancel_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{140.0f, kButtonHeight},
      "Cancel", [this]() { CloseCreateModal(); });

  modal_elements_.push_back(modal_room_code_input_);
  modal_elements_.push_back(modal_max_players_input_);
  modal_elements_.push_back(modal_privacy_button_);
  modal_elements_.push_back(modal_create_button_);
  modal_elements_.push_back(modal_cancel_button_);
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
      set_focus(modal_room_code_input_);
      set_focus(modal_max_players_input_);
    }
  }

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
  for (auto& elem : room_buttons_) {
    elem->Update(dt, input);
  }
  if (show_modal_) {
    for (auto& elem : modal_elements_) {
      elem->Update(dt, input);
    }
  }

  RefreshRoomButtons();

  if (auto created = app_.ConsumeLastRoomCreation()) {
    if (created->success && created->room) {
      modal_room_code_input_->SetText(created->room->room_code);
    }
  }
}

void LobbyScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);
  const auto window_size = app_.GetEngine().Window().GetSize();
  const engine::render::Color white = engine::render::Color::White();
  renderer.DrawText("Find a Room", {32.0f, 60.0f}, 40.0f, white);
  renderer.DrawText(app_.RoomDirectoryStatus(), {32.0f, 100.0f}, 20.0f, white);
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  for (auto& elem : room_buttons_) {
    elem->Draw(renderer);
  }

  if (show_modal_) {
    const engine::render::Color overlay =
        engine::render::Color::FromBytes(0, 0, 0, 180);
    renderer.DrawRect({0.0f, 0.0f, static_cast<float>(window_size.x),
                       static_cast<float>(window_size.y)},
                      overlay);
    const float modal_width = 520.0f;
    const float modal_height = 280.0f;
    const float modal_x =
        (static_cast<float>(window_size.x) - modal_width) * 0.5f;
    const float modal_y =
        (static_cast<float>(window_size.y) - modal_height) * 0.5f;
    renderer.DrawRect({modal_x, modal_y, modal_width, modal_height},
                      engine::render::Color::FromBytes(20, 20, 30, 240));
    for (auto& elem : modal_elements_) {
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
    const std::string label = room.room_code + " | " +
                              (room.is_private ? "Private" : "Public") + " | " +
                              std::to_string(room.player_count) + "/" +
                              std::to_string(room.max_players);
    auto button = std::make_shared<ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{0.0f, kRoomButtonHeight}, label, [this, room]() {
          std::string host = host_input_->GetText();
          std::string port_str = port_input_->GetText();
          std::string name = name_input_->GetText();
          if (host.empty()) host = "127.0.0.1";
          if (port_str.empty()) port_str = "4242";
          if (name.empty()) name = "Pilot";
          try {
            const int port = std::stoi(port_str);
            if (port < 1 || port > 65535) {
              LogLifecycle(engine::util::LogLevel::kWarn,
                           "Port must be between 1 and 65535");
              return;
            }
            app_.SetConnectionConfig(host, port, name, room.room_code);
            app_.StartConnection();
          } catch (const std::exception& e) {
            LogLifecycle(engine::util::LogLevel::kWarn,
                         std::string("Invalid port: ") + e.what());
          }
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
  const float margin = 32.0f;
  const float list_top = 220.0f;
  const float list_width = width - margin * 2.0f;

  host_input_->SetSize({260.0f, kFieldHeight});
  port_input_->SetSize({120.0f, kFieldHeight});
  name_input_->SetSize({200.0f, kFieldHeight});

  float x = margin;
  const float y = 140.0f;
  host_input_->SetPosition({x, y});
  x += host_input_->GetSize().x + 12.0f;
  port_input_->SetPosition({x, y});
  x += port_input_->GetSize().x + 12.0f;
  name_input_->SetPosition({x, y});
  x += name_input_->GetSize().x + 12.0f;
  refresh_button_->SetPosition({x, y - 6.0f});
  x += refresh_button_->GetSize().x + 12.0f;
  create_button_->SetPosition({x, y - 6.0f});
  x += create_button_->GetSize().x + 12.0f;
  back_button_->SetPosition({x, y - 6.0f});

  const auto room_area_width = list_width;
  float room_y = list_top;
  for (auto& button : room_buttons_) {
    button->SetPosition({margin, room_y});
    button->SetSize({room_area_width, kRoomButtonHeight});
    room_y += kRoomButtonHeight + 8.0f;
  }

  if (show_modal_) {
    ApplyModalLayout();
  }
}

void LobbyScene::OpenCreateModal() { show_modal_ = true; }

void LobbyScene::CloseCreateModal() {
  show_modal_ = false;
  modal_private_ = false;
  modal_privacy_button_->SetText("Public");
}

void LobbyScene::ApplyModalLayout() {
  const auto window_size = app_.GetEngine().Window().GetSize();
  const float modal_width = 520.0f;
  const float modal_height = 280.0f;
  const float x = (static_cast<float>(window_size.x) - modal_width) * 0.5f;
  const float y = (static_cast<float>(window_size.y) - modal_height) * 0.5f;

  modal_room_code_input_->SetPosition({x + 32.0f, y + 36.0f});
  modal_room_code_input_->SetSize({modal_width - 64.0f, kFieldHeight});

  modal_max_players_input_->SetPosition(
      {x + 32.0f, y + 36.0f + kFieldHeight + 12.0f});
  modal_max_players_input_->SetSize({180.0f, kFieldHeight});

  modal_privacy_button_->SetPosition(
      {x + 32.0f + 200.0f, modal_max_players_input_->GetPosition().y - 2.0f});
  modal_privacy_button_->SetSize({160.0f, kButtonHeight});

  modal_create_button_->SetPosition({x + 32.0f, y + modal_height - 76.0f});
  modal_cancel_button_->SetPosition(
      {x + modal_width - modal_cancel_button_->GetSize().x - 32.0f,
       y + modal_height - 76.0f});
  modal_create_button_->SetSize({140.0f, kButtonHeight});
  modal_cancel_button_->SetSize({140.0f, kButtonHeight});
}

}  // namespace client
