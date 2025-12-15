#include "main_menu_scene.h"

#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {
constexpr float kButtonHeight = 72.0f;
constexpr float kButtonWidth = 420.0f;
}  // namespace

MainMenuScene::MainMenuScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();

  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  const auto white = engine::render::Color::White();

  play_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Play",
      [this]() { app_.OnPlay(); });
  settings_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Settings",
      [this]() { app_.OnOpenSettings(); });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Quit",
      [this]() { app_.OnQuitApplication(); });

  ui_elements_.push_back(play_button_);
  ui_elements_.push_back(settings_button_);
  ui_elements_.push_back(quit_button_);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(48.0f));
  root->SetSpacing(36.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_text_ = std::make_shared<engine::ui::TextElement>(
      "R-TYPE", engine::ui::FontSize::RelativeWidth(0.12f), white);
  title_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text_);

  auto subtitle = std::make_shared<engine::ui::TextElement>(
      "Multiplayer Arcade", engine::ui::FontSize::RelativeWidth(0.03f), white);
  subtitle->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(subtitle);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(22.0f);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(kButtonHeight + 8.0f);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition(
          {rect.top_left_x_ + (rect.width_ - kButtonWidth) * 0.5f,
           rect.top_left_y_ + 4.0f});
      button->SetSize({kButtonWidth, kButtonHeight});
    });
    button_column->AddChild(slot);
  };

  add_slot(play_button_);
  add_slot(settings_button_);
  add_slot(quit_button_);

  root->AddChild(button_column);

  canvas_.SetRoot(root);

  const auto btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  if (btn_tex) {
    play_button_->SetTexture(btn_tex);
    settings_button_->SetTexture(btn_tex);
    quit_button_->SetTexture(btn_tex);
    const auto hover = engine::render::Color::FromBytes(220, 220, 220);
    const auto press = engine::render::Color::FromBytes(180, 180, 180);
    play_button_->SetColors(white, hover, press);
    settings_button_->SetColors(white, hover, press);
    quit_button_->SetColors(white, hover, press);
  }
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();

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
