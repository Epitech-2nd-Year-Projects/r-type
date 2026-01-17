#include "game_over_scene.h"

#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {
GameOverScene::GameOverScene(ClientContext& context, const Stats& stats)
    : context_(context), stats_(stats) {
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
      "GAME OVER",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::GameOver::kTitleFontScale),
      constants::ui::GameOver::kTitleColor);

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

  menu_main_exit_ = std::make_shared<engine::ui::TextElement>(
      "Return to Main Menu",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::GameOver::kMenuFontScale),
      constants::ui::GameOver::kSelectedColor);

  root->AddChild(title_);
  root->AddChild(score_text_);
  root->AddChild(wave_level_text_);

  auto spacer = std::make_shared<engine::ui::BoxElement>();
  spacer->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::GameOver::kSpacerHeight);
  root->AddChild(spacer);

  root->AddChild(menu_main_exit_);

  canvas_.SetRoot(root);
  UpdateMenuVisuals();
}

void GameOverScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = context_.Input();

  if (input.IsActionActive(std::string(constants::input::kActionConfirm))) {
    context_.OnQuitToMenu();
  }
}

void GameOverScene::UpdateMenuVisuals() {
  menu_main_exit_->SetColor(constants::ui::GameOver::kSelectedColor);
}

void GameOverScene::Draw(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
