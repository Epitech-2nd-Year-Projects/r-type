#include "main_menu_scene.h"

#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/render/renderer2d.h"
#include "engine/math/rect.h"

namespace client {

MainMenuScene::MainMenuScene(Application& app) : app_(app) {
    auto& renderer = app_.GetEngine().Renderer();
    
    renderer.LoadFont("kenney_future", "assets/kenney_ui-pack-space-expansion/Font/Kenney Future.ttf");
    renderer.SetFont("kenney_future");

    auto white = engine::render::Color::White();
    
    float center_x = 1600.0f / 2.0f;
    float start_y = 200.0f;
    
    ui_elements_.push_back(std::make_shared<ui::Label>(
        engine::math::Vector2f{center_x - 120.0f, 80.0f}, "R-Type", 60.0f, white));

    ui_elements_.push_back(std::make_shared<ui::Label>(
        engine::math::Vector2f{center_x - 200.0f, start_y}, "Server Address", 20.0f, white));
    host_input_ = std::make_shared<ui::TextInput>(
        engine::math::Vector2f{center_x - 200.0f, start_y + 25.0f}, engine::math::Vector2f{400.0f, 40.0f});
    host_input_->SetPlaceholder("127.0.0.1");
    host_input_->SetText(app.GetEngine().Config().GetString("client.host", "127.0.0.1"));
    ui_elements_.push_back(host_input_);

    start_y += 90.0f;
    ui_elements_.push_back(std::make_shared<ui::Label>(
        engine::math::Vector2f{center_x - 200.0f, start_y}, "Port", 20.0f, white));
    port_input_ = std::make_shared<ui::TextInput>(
        engine::math::Vector2f{center_x - 200.0f, start_y + 25.0f}, engine::math::Vector2f{400.0f, 40.0f});
    port_input_->SetPlaceholder("5000");
    port_input_->SetText(app.GetEngine().Config().GetString("client.port", "5000"));
    ui_elements_.push_back(port_input_);
    
    start_y += 90.0f;
    ui_elements_.push_back(std::make_shared<ui::Label>(
        engine::math::Vector2f{center_x - 200.0f, start_y}, "Nickname", 20.0f, white));
    name_input_ = std::make_shared<ui::TextInput>(
        engine::math::Vector2f{center_x - 200.0f, start_y + 25.0f}, engine::math::Vector2f{400.0f, 40.0f});
    name_input_->SetPlaceholder("Player");
    name_input_->SetText(app.GetEngine().Config().GetString("client.player_name", "Player"));
    ui_elements_.push_back(name_input_);

    start_y += 100.0f;
    auto connect_btn = std::make_shared<ui::Button>(
        engine::math::Vector2f{center_x - 200.0f, start_y}, engine::math::Vector2f{400.0f, 50.0f},
        "Connect",
        [this]() {
            std::string host = host_input_->GetText();
            std::string port_str = port_input_->GetText();
            std::string name = name_input_->GetText();
            if (host.empty()) host = "127.0.0.1";
            if (port_str.empty()) port_str = "5000";
            if (name.empty()) name = "Player";
            
            try {
                int port = std::stoi(port_str);
                app_.SetConnectionConfig(host, port, name);
                app_.StartConnection();
            } catch (...) {
            }
        });
    ui_elements_.push_back(connect_btn);

    start_y += 70.0f;
    auto settings_btn = std::make_shared<ui::Button>(
        engine::math::Vector2f{center_x - 200.0f, start_y}, engine::math::Vector2f{400.0f, 50.0f},
        "Settings",
        [this]() {
            app_.OnOpenSettings();
        });
    ui_elements_.push_back(settings_btn);
    
    auto btn_tex = renderer.LoadTextureFromFile("assets/kenney_ui-pack-space-expansion/PNG/Grey/Default/button_square_header_large_rectangle.png");
    if (btn_tex) {
        connect_btn->SetTexture(btn_tex);
        settings_btn->SetTexture(btn_tex);
        connect_btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220), engine::render::Color::FromBytes(180, 180, 180));
        settings_btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220), engine::render::Color::FromBytes(180, 180, 180));
    }
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
    auto& input = app_.GetEngine().Input();
    
    bool is_down = input.IsMouseButtonDown(engine::input::MouseButton::kLeft);
    if (is_down && !was_mouse_down_) {
        auto pos = input.GetMousePosition();
        
        auto check = [&](std::shared_ptr<ui::TextInput>& field) {
             engine::math::RectF rect{field->GetPosition(), field->GetSize()};
             field->SetFocused(rect.Contains(pos));
        };
        
        check(host_input_);
        check(port_input_);
        check(name_input_);
    }
    was_mouse_down_ = is_down;

    if (input.IsKeyDown(engine::input::Key::kEnter)) {
    }

    for (auto& elem : ui_elements_) {
        elem->Update(dt, input);
    }
}

void MainMenuScene::Draw(engine::render::Renderer2D& renderer) {
  for (auto& elem : ui_elements_) {
      elem->Draw(renderer);
  }
}

}  // namespace client