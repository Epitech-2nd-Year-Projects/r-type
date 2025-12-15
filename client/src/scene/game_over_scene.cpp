#include "game_over_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {
namespace {
const engine::render::Color kNormalColor = engine::render::Color::White();
const engine::render::Color kSelectedColor =
    engine::render::Color::FromBytes(255, 215, 0);  // Gold
}  // namespace

GameOverScene::GameOverScene(Application& app, const Stats& stats)
    : app_(app), stats_(stats) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(20.0f);  // Increased spacing
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "GAME OVER", engine::ui::FontSize::RelativeWidth(0.08f),
      engine::render::Color::FromBytes(255, 50, 50));

  score_text_ = std::make_shared<engine::ui::TextElement>(
      "Final Score: " + std::to_string(stats_.score),
      engine::ui::FontSize::RelativeWidth(0.04f), kNormalColor);

  wave_level_text_ = std::make_shared<engine::ui::TextElement>(
      "Level " + std::to_string(stats_.level) + " - Wave " +
          std::to_string(stats_.wave),
      engine::ui::FontSize::RelativeWidth(0.03f), kNormalColor);

  menu_main_exit_ = std::make_shared<engine::ui::TextElement>(
      "Return to Main Menu", engine::ui::FontSize::RelativeWidth(0.03f),
      kSelectedColor);

  root->AddChild(title_);
  root->AddChild(score_text_);
  root->AddChild(wave_level_text_);
  
  // Spacer
  auto spacer = std::make_shared<engine::ui::BoxElement>();
  spacer->Layout().size.height = engine::ui::LayoutValue::Pixels(40.0f);
  root->AddChild(spacer);

  root->AddChild(menu_main_exit_);

  canvas_.SetRoot(root);
  UpdateMenuVisuals();
}

void GameOverScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    // Only one option for now, so Confirm always goes to menu
    app_.OnQuitToMenu();
  }
}

void GameOverScene::UpdateMenuVisuals() {
    // Single option, always selected
    menu_main_exit_->SetColor(kSelectedColor);
}

void GameOverScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
