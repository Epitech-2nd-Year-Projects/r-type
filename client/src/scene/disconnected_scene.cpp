#include "disconnected_scene.h"

#include <utility>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {

DisconnectedScene::DisconnectedScene(Application& app, std::string reason)
    : app_(app), reason_(std::move(reason)) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(14.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "Disconnected", engine::ui::FontSize::RelativeWidth(0.06f),
      engine::render::Color::FromBytes(255, 0, 0));
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  reason_text_ = std::make_shared<engine::ui::TextElement>(
      reason_, engine::ui::FontSize::RelativeWidth(0.03f),
      engine::render::Color::White());
  reason_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  action_text_ = std::make_shared<engine::ui::TextElement>(
      "Press ENTER to Main Menu", engine::ui::FontSize::RelativeWidth(0.03f),
      engine::render::Color::White());
  action_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  root->AddChild(title_);
  root->AddChild(reason_text_);
  root->AddChild(action_text_);

  canvas_.SetRoot(root);
}

void DisconnectedScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();

  if (input.IsActionActive("Confirm")) {
    app_.OnQuitToMenu();
  }
}

void DisconnectedScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
