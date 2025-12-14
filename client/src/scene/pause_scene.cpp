#include "pause_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {

PauseScene::PauseScene(Application& app) : app_(app) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(12.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "Paused", engine::ui::FontSize::RelativeWidth(0.06f),
      engine::render::Color::White());
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  resume_ = std::make_shared<engine::ui::TextElement>(
      "Press ENTER to Resume", engine::ui::FontSize::RelativeWidth(0.03f),
      engine::render::Color::White());
  resume_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  quit_ = std::make_shared<engine::ui::TextElement>(
      "Press Q to Quit to Main Menu",
      engine::ui::FontSize::RelativeWidth(0.03f),
      engine::render::Color::White());
  quit_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  root->AddChild(title_);
  root->AddChild(resume_);
  root->AddChild(quit_);

  canvas_.SetRoot(root);
}

void PauseScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm") || input.IsActionActive("Cancel")) {
    app_.OnGameResume();
  } else if (input.IsActionActive("Quit")) {
    app_.OnQuitToMenu();
  }
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
