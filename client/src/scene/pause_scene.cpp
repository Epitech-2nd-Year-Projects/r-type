#include "pause_scene.h"

#include "application.h"
#include "engine/app/engine_runtime.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace client {
namespace {

constexpr float kButtonWidth = 360.0f;
constexpr float kButtonHeight = 64.0f;
}  // namespace

PauseScene::PauseScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  auto& input = app_.GetEngine().Input();
  pause_toggle_pressed_ =
      input.IsActionActive("Pause") || input.IsActionActive("Cancel");
  confirm_pressed_ = input.IsActionActive("Confirm");

  const auto white = engine::render::Color::White();
  const auto hover = engine::render::Color::FromBytes(220, 220, 220);
  const auto press = engine::render::Color::FromBytes(180, 180, 180);

  resume_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Resume",
      [this]() { app_.OnGameResume(); });
  options_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Options",
      [this]() { app_.OnOpenSettings(); });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Quit to Main Menu",
      [this]() { app_.OnQuitToMenu(); });

  menu_buttons_.push_back(resume_button_);
  menu_buttons_.push_back(options_button_);
  menu_buttons_.push_back(quit_button_);

  const auto large_button_texture =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  if (large_button_texture) {
    for (auto& button : menu_buttons_) {
      button->SetTexture(large_button_texture);
      button->SetColors(white, hover, press);
    }
  }

  title_ = std::make_shared<engine::ui::TextElement>(
      "Paused", engine::ui::FontSize::RelativeWidth(0.06f),
      engine::render::Color::White());
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  BuildUi();
}

void PauseScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();

  for (auto& button : menu_buttons_) {
    button->Update(dt, input);
  }

  const bool pause_toggle =
      input.IsActionActive("Pause") || input.IsActionActive("Cancel");
  if (pause_toggle && !pause_toggle_pressed_) {
    app_.OnGameResume();
  }
  pause_toggle_pressed_ = pause_toggle;

  const bool confirm_pressed = input.IsActionActive("Confirm");
  if (confirm_pressed && !confirm_pressed_) {
    app_.OnGameResume();
  }
  confirm_pressed_ = confirm_pressed;
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  renderer.DrawRect({0.0f, 0.0f, static_cast<float>(window_size.x),
                     static_cast<float>(window_size.y)},
                    engine::render::Color::FromBytes(6, 10, 22, 210));

  canvas_.Draw(renderer);
  for (auto& button : menu_buttons_) {
    button->Draw(renderer);
  }
}

void PauseScene::BuildUi() {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(28.0f));
  root->SetSpacing(14.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  root->AddChild(title_);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(10.0f);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.width = engine::ui::LayoutValue::Pixels(kButtonWidth);
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(kButtonHeight + 8.0f);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      const float x = rect.top_left_x_;
      const float y = rect.top_left_y_ + (rect.height_ - kButtonHeight) * 0.5f;
      button->SetPosition({x, y});
      button->SetSize({rect.width_, kButtonHeight});
    });
    button_column->AddChild(slot);
  };

  add_slot(resume_button_);
  add_slot(options_button_);
  add_slot(quit_button_);

  root->AddChild(button_column);

  canvas_.SetRoot(root);
}

void PauseScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
