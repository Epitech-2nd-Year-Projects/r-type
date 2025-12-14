#include "connecting_scene.h"

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "join_flow.h"

namespace client {

ConnectingScene::ConnectingScene(Application& app) : app_(app) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  status_text_ = std::make_shared<engine::ui::TextElement>(
      app_.GetJoinFlow().status(), engine::ui::FontSize::RelativeWidth(0.04f),
      engine::render::Color::White());
  status_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(status_text_);

  canvas_.SetRoot(root);
}

void ConnectingScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = app_.GetEngine().Input();
  if (input.IsActionActive("Cancel")) {
    app_.OnQuitToMenu();
  }
}

void ConnectingScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  status_text_->SetText(app_.GetJoinFlow().status());
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
