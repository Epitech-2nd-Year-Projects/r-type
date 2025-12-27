#include "pause_scene.h"

#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace client {
PauseScene::PauseScene(ClientContext& context) : context_(context) {
  auto& renderer = context_.Renderer();
  renderer.LoadFont(std::string(constants::ui::kTitleFont),
                    std::string(constants::ui::kTitleFontPath));
  renderer.LoadFont(std::string(constants::ui::kBodyFont),
                    std::string(constants::ui::kBodyFontPath));
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  auto& input = context_.Input();
  pause_toggle_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
  confirm_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionConfirm));

  const auto white = constants::ui::kButtonBaseColor;
  const auto hover = constants::ui::kButtonHoverColor;
  const auto press = constants::ui::kButtonPressColor;

  resume_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Pause::kButtonWidth,
                             constants::ui::Pause::kButtonHeight},
      "Resume", [this]() { context_.OnGameResume(); });
  options_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Pause::kButtonWidth,
                             constants::ui::Pause::kButtonHeight},
      "Options", [this]() { context_.OnOpenSettings(); });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Pause::kButtonWidth,
                             constants::ui::Pause::kButtonHeight},
      "Quit to Main Menu", [this]() { context_.OnQuitToMenu(); });

  menu_buttons_.push_back(resume_button_);
  menu_buttons_.push_back(options_button_);
  menu_buttons_.push_back(quit_button_);

  const auto large_button_texture = renderer.LoadTextureFromFile(
      std::string(constants::ui::kButtonTextureLargePath));
  if (large_button_texture) {
    for (auto& button : menu_buttons_) {
      button->SetTexture(large_button_texture);
      button->SetColors(white, hover, press);
    }
  }

  title_ = std::make_shared<engine::ui::TextElement>(
      "Paused",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Pause::kTitleFontScale),
      engine::render::Color::White());
  title_->SetFont(std::string(constants::ui::kTitleFont));
  title_->SetFontFallback(std::string(constants::ui::kBodyFont));
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  BuildUi();
}

void PauseScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();

  for (auto& button : menu_buttons_) {
    button->Update(dt, input);
  }

  const bool pause_toggle =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
  if (pause_toggle && !pause_toggle_pressed_) {
    context_.OnGameResume();
  }
  pause_toggle_pressed_ = pause_toggle;

  const bool confirm_pressed =
      input.IsActionActive(std::string(constants::input::kActionConfirm));
  if (confirm_pressed && !confirm_pressed_) {
    context_.OnGameResume();
  }
  confirm_pressed_ = confirm_pressed;
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  renderer.DrawRect({0.0f, 0.0f, static_cast<float>(window_size.x),
                     static_cast<float>(window_size.y)},
                    constants::ui::Pause::kOverlayColor);

  renderer.SetFont(std::string(constants::ui::kBodyFont));
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
  root->SetPadding(
      engine::ui::Insets::Uniform(constants::ui::Pause::kRootPadding));
  root->SetSpacing(constants::ui::Pause::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  root->AddChild(title_);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(constants::ui::Pause::kButtonColumnSpacing);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.width =
        engine::ui::LayoutValue::Pixels(constants::ui::Pause::kButtonWidth);
    slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
        constants::ui::Pause::kButtonHeight +
        constants::ui::Pause::kButtonSlotPadding);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      const float x = rect.top_left_x_;
      const float y =
          rect.top_left_y_ +
          (rect.height_ - constants::ui::Pause::kButtonHeight) * 0.5f;
      button->SetPosition({x, y});
      button->SetSize({rect.width_, constants::ui::Pause::kButtonHeight});
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
  const auto window_size = context_.Window().GetSize();
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
