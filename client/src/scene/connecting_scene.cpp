#include "connecting_scene.h"

#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {

ConnectingScene::ConnectingScene(ClientContext& context) : context_(context) {
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
      std::string(context_.ConnectionStatus()),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Connecting::kStatusFontScale),
      engine::render::Color::White());
  status_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(status_text_);

  canvas_.SetRoot(root);
}

void ConnectingScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = context_.Input();
  if (input.IsActionActive(std::string(constants::input::kActionCancel))) {
    context_.OnQuitToMenu();
  }
}

void ConnectingScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  status_text_->SetText(std::string(context_.ConnectionStatus()));
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
