#include "main_menu_scene.h"

#include <exception>
#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

MainMenuScene::MainMenuScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();

  renderer.LoadFont("times", "assets/times.ttf");
  renderer.SetFont("times");

  const auto white = engine::render::Color::White();

  host_input_ =
      std::make_shared<ui::TextInput>(engine::math::Vector2f{0.0f, 0.0f},
                                      engine::math::Vector2f{400.0f, 40.0f});
  host_input_->SetPlaceholder("127.0.0.1");
  host_input_->SetText(
      app.GetEngine().Config().GetString("client.host", "127.0.0.1"));
  port_input_ =
      std::make_shared<ui::TextInput>(engine::math::Vector2f{0.0f, 0.0f},
                                      engine::math::Vector2f{400.0f, 40.0f});
  port_input_->SetPlaceholder("5000");
  port_input_->SetText(
      app.GetEngine().Config().GetString("client.port", "5000"));
  name_input_ =
      std::make_shared<ui::TextInput>(engine::math::Vector2f{0.0f, 0.0f},
                                      engine::math::Vector2f{400.0f, 40.0f});
  name_input_->SetPlaceholder("Player");
  name_input_->SetText(
      app.GetEngine().Config().GetString("client.player_name", "Player"));
  connect_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f}, engine::math::Vector2f{400.0f, 50.0f},
      "Connect", [this]() {
        std::string host = host_input_->GetText();
        std::string port_str = port_input_->GetText();
        std::string name = name_input_->GetText();
        if (host.empty()) host = "127.0.0.1";
        if (port_str.empty()) port_str = "5000";
        if (name.empty()) name = "Player";

        try {
          int port = std::stoi(port_str);
          if (port < 1 || port > 65535) {
            LogLifecycle(engine::util::LogLevel::kWarn,
                         "Port must be between 1 and 65535");
            return;
          }
          app_.SetConnectionConfig(host, port, name);
          app_.StartConnection();
        } catch (const std::exception& e) {
          const std::string message = std::string("Invalid port: ") + e.what();
          LogLifecycle(engine::util::LogLevel::kWarn, message);
        }
      });
  settings_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f}, engine::math::Vector2f{400.0f, 50.0f},
      "Settings", [this]() { app_.OnOpenSettings(); });

  ui_elements_.push_back(host_input_);
  ui_elements_.push_back(port_input_);
  ui_elements_.push_back(name_input_);
  ui_elements_.push_back(connect_button_);
  ui_elements_.push_back(settings_button_);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(24.0f));
  root->SetSpacing(18.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kStart});

  title_text_ = std::make_shared<engine::ui::TextElement>(
      "R-TYPE", engine::ui::FontSize::RelativeWidth(0.08f), white);
  title_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text_);

  auto form =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  form->SetSpacing(16.0f);
  form->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  form->Layout().size.width = engine::ui::LayoutValue::Percent(0.65f);

  auto build_field = [&](const std::string& label_text,
                         const std::shared_ptr<ui::TextInput>& input) {
    auto field = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kVertical);
    field->SetSpacing(6.0f);
    field->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kStretch;

    auto label = std::make_shared<engine::ui::TextElement>(
        label_text, engine::ui::FontSize::RelativeWidth(0.025f), white);
    label->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kStart;

    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kStretch;
    slot->Layout().size.height = engine::ui::LayoutValue::Percent(0.08f);
    slot->SetLayoutCallback([input](const engine::math::RectF& rect) {
      input->SetPosition({rect.top_left_x_, rect.top_left_y_});
      input->SetSize({rect.width_, rect.height_});
    });

    field->AddChild(label);
    field->AddChild(slot);
    form->AddChild(field);
  };

  build_field("Server Address", host_input_);
  build_field("Port", port_input_);
  build_field("Nickname", name_input_);

  auto actions =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  actions->SetSpacing(12.0f);
  actions->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;

  auto connect_slot = std::make_shared<engine::ui::BoxElement>();
  connect_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  connect_slot->Layout().size.height = engine::ui::LayoutValue::Percent(0.08f);
  connect_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    connect_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    connect_button_->SetSize({rect.width_, rect.height_});
  });
  actions->AddChild(connect_slot);

  auto settings_slot = std::make_shared<engine::ui::BoxElement>();
  settings_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  settings_slot->Layout().size.height = engine::ui::LayoutValue::Percent(0.08f);
  settings_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    settings_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    settings_button_->SetSize({rect.width_, rect.height_});
  });
  actions->AddChild(settings_slot);

  form->AddChild(actions);
  root->AddChild(form);

  canvas_.SetRoot(root);

  auto btn_tex = renderer.LoadTextureFromFile("assets/ui/button_large.png");
  if (btn_tex) {
    connect_button_->SetTexture(btn_tex);
    settings_button_->SetTexture(btn_tex);
    connect_button_->SetColors(white,
                               engine::render::Color::FromBytes(220, 220, 220),
                               engine::render::Color::FromBytes(180, 180, 180));
    settings_button_->SetColors(
        white, engine::render::Color::FromBytes(220, 220, 220),
        engine::render::Color::FromBytes(180, 180, 180));
  }
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
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

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
}

void MainMenuScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
}

void MainMenuScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
