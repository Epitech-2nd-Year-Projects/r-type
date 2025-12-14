#include "game_over_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {

GameOverScene::GameOverScene(Application& app) : app_(app) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(10.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "Game Over", engine::ui::FontSize::RelativeWidth(0.06f),
      engine::render::Color::FromBytes(255, 0, 0));
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  prompt_ = std::make_shared<engine::ui::TextElement>(
      "Press ENTER to Main Menu", engine::ui::FontSize::RelativeWidth(0.03f),
      engine::render::Color::White());
  prompt_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  root->AddChild(title_);
  root->AddChild(prompt_);

  canvas_.SetRoot(root);
}

void GameOverScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    app_.OnQuitToMenu();
  }
}

void GameOverScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
