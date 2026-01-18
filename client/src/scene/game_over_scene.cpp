#include "game_over_scene.h"

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "ui/menu_background.h"

namespace client {

namespace {

ui::MenuPointerConfig GameOverPointerConfig() {
  return ui::MenuPointerConfig{constants::ui::kMenuPointerFramePrefix,
                               constants::ui::kMenuPointerFrameExtension,
                               constants::ui::MainMenu::kPointerFrameCount,
                               constants::ui::MainMenu::kPointerFrameDuration,
                               constants::ui::MainMenu::kPointerHeightFactor,
                               60.0f,
                               constants::ui::MainMenu::kPointerScaleFactor};
}

}  // namespace

GameOverScene::GameOverScene(ClientContext& context, const Stats& stats)
    : context_(context),
      stats_(stats),
      menu_effects_(context, GameOverPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();

  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  menu_effects_.Load();

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(constants::ui::GameOver::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "MISSION FAILED",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::GameOver::kTitleFontScale),
      constants::ui::GameOver::kSelectedColor);
  title_->SetFont(std::string(constants::ui::kTitleFont));

  score_text_ = std::make_shared<engine::ui::TextElement>(
      "Final Score: " + std::to_string(stats_.score),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::GameOver::kScoreFontScale),
      constants::ui::GameOver::kNormalColor);

  wave_level_text_ = std::make_shared<engine::ui::TextElement>(
      "Wave " + std::to_string(stats_.wave),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::GameOver::kWaveFontScale),
      constants::ui::GameOver::kNormalColor);

  menu_main_exit_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{250.0f, constants::ui::MainMenu::kButtonHeight},
      "Return to Menu",
      menu_effects_.WrapClick([this]() { context_.OnQuitToMenu(); }));

  ui_elements_.push_back(menu_main_exit_button_);

  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);
  const auto white = engine::render::Color::White();
  menu_main_exit_button_->SetColors(transparent, transparent, transparent);
  menu_main_exit_button_->SetTextColor(white);
  menu_main_exit_button_->SetTextScale(
      constants::ui::MainMenu::kButtonTextScale);

  root->AddChild(title_);
  root->AddChild(score_text_);
  root->AddChild(wave_level_text_);

  auto spacer = std::make_shared<engine::ui::BoxElement>();
  spacer->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::GameOver::kSpacerHeight);
  root->AddChild(spacer);

  auto button_slot = std::make_shared<engine::ui::BoxElement>();
  button_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(250.0f);
  button_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::MainMenu::kButtonHeight);
  button_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    menu_main_exit_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    menu_main_exit_button_->SetSize({rect.width_, rect.height_});
  });

  root->AddChild(button_slot);

  canvas_.SetRoot(root);
}

void GameOverScene::Update(engine::time::TimeDelta dt) {
  context_.MenuBackground().Update(dt);

  auto& input = context_.Input();
  menu_effects_.Update(dt, input, ui_elements_);

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }

  if (input.IsActionActive(std::string(constants::input::kActionConfirm))) {
    context_.OnQuitToMenu();
  }
}

void GameOverScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void GameOverScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  context_.MenuBackground().Draw(context_.RenderSize());
}

void GameOverScene::DrawForeground(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  LayoutUi(renderer);
  canvas_.Draw(renderer);

  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  menu_effects_.DrawPointers(renderer, ui_elements_);
}

void GameOverScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client