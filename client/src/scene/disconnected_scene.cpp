#include "disconnected_scene.h"

#include <utility>

#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"

namespace client {

DisconnectedScene::DisconnectedScene(ClientContext& context, std::string reason)
    : context_(context), reason_(std::move(reason)) {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(constants::ui::Disconnected::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  title_ = std::make_shared<engine::ui::TextElement>(
      "Disconnected",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Disconnected::kTitleFontScale),
      constants::ui::Disconnected::kTitleColor);
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  reason_text_ = std::make_shared<engine::ui::TextElement>(
      reason_,
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Disconnected::kReasonFontScale),
      engine::render::Color::White());
  reason_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  action_text_ = std::make_shared<engine::ui::TextElement>(
      "Press ENTER to Main Menu",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Disconnected::kActionFontScale),
      engine::render::Color::White());
  action_text_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  root->AddChild(title_);
  root->AddChild(reason_text_);
  root->AddChild(action_text_);

  canvas_.SetRoot(root);
}

void DisconnectedScene::Update(engine::time::TimeDelta /*dt*/) {
  auto& input = context_.Input();

  if (input.IsActionActive(std::string(constants::input::kActionConfirm))) {
    context_.OnQuitToMenu();
  }
}

void DisconnectedScene::Draw(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.LayoutAndDraw(renderer);
}

}  // namespace client
