#include "main_menu_scene.h"

#include <algorithm>
#include <string>
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"

namespace client {

namespace {

ui::MenuPointerConfig MainMenuPointerConfig() {
  return ui::MenuPointerConfig{constants::ui::kMenuPointerFramePrefix,
                               constants::ui::kMenuPointerFrameExtension,
                               constants::ui::MainMenu::kPointerFrameCount,
                               constants::ui::MainMenu::kPointerFrameDuration,
                               constants::ui::MainMenu::kPointerHeightFactor,
                               constants::ui::MainMenu::kPointerSpacing,
                               constants::ui::MainMenu::kPointerScaleFactor};
}

}  // namespace

MainMenuScene::MainMenuScene(ClientContext& context)
    : context_(context),
      menu_effects_(context, MainMenuPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& renderer = context_.Renderer();

  renderer.LoadFont(std::string(constants::ui::kTitleFont),
                    std::string(constants::ui::kTitleFontPath));
  renderer.LoadFont(std::string(constants::ui::kBodyFont),
                    std::string(constants::ui::kBodyFontPath));
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  menu_effects_.Load(renderer);
  title_texture_ = renderer.LoadTextureFromFile(
      std::string(constants::ui::MainMenu::kTitleTexturePath));

  const auto white = engine::render::Color::White();

  play_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Start Game",
      menu_effects_.WrapClick([this]() { context_.OnPlay(); }));
  settings_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Options",
      menu_effects_.WrapClick([this]() { context_.OnOpenSettings(); }));
  quit_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Quit Game",
      menu_effects_.WrapClick([this]() { context_.OnQuitApplication(); }));

  ui_elements_.push_back(play_button_);
  ui_elements_.push_back(settings_button_);
  ui_elements_.push_back(quit_button_);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(
      engine::ui::Insets::Uniform(constants::ui::MainMenu::kRootPadding));
  root->SetSpacing(constants::ui::MainMenu::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_slot = std::make_shared<engine::ui::BoxElement>();
  title_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  title_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::MainMenu::kTitleSlotHeight);
  title_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  title_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    title_rect_ = {rect.top_left_x_,
                   rect.top_left_y_ + constants::ui::MainMenu::kTitleYOffset,
                   rect.width_, rect.height_};
  });
  root->AddChild(title_slot);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(constants::ui::MainMenu::kButtonColumnSpacing);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<engine::ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
        constants::ui::MainMenu::kButtonHeight +
        constants::ui::MainMenu::kButtonSlotPadding);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition(
          {rect.top_left_x_ +
               (rect.width_ - constants::ui::MainMenu::kButtonWidth) * 0.5f,
           rect.top_left_y_ + constants::ui::MainMenu::kButtonSlotInset});
      button->SetSize({constants::ui::MainMenu::kButtonWidth,
                       constants::ui::MainMenu::kButtonHeight});
    });
    button_column->AddChild(slot);
  };

  add_slot(play_button_);
  add_slot(settings_button_);
  add_slot(quit_button_);

  root->AddChild(button_column);

  canvas_.SetRoot(root);
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);
  play_button_->SetColors(transparent, transparent, transparent);
  settings_button_->SetColors(transparent, transparent, transparent);
  quit_button_->SetColors(transparent, transparent, transparent);
  play_button_->SetTextColor(white);
  settings_button_->SetTextColor(white);
  quit_button_->SetTextColor(white);
  play_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
  settings_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
  quit_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();

  menu_effects_.Update(dt, input, ui_elements_);

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
}

void MainMenuScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);

  canvas_.Draw(renderer);
  DrawTitle(renderer);
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  menu_effects_.DrawPointers(renderer, ui_elements_);
}

void MainMenuScene::LayoutUi(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  const auto window_size = context_.Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

void MainMenuScene::DrawTitle(engine::render::Renderer2D& renderer) {
  if (!title_texture_) {
    return;
  }
  const auto tex_size = title_texture_->GetSize();
  if (tex_size.y == 0) {
    return;
  }
  const float scale =
      std::min(title_rect_.width_ / static_cast<float>(tex_size.x),
               title_rect_.height_ / static_cast<float>(tex_size.y));
  if (scale <= 0.0f) {
    return;
  }
  const float draw_scale = scale * constants::ui::MainMenu::kTitleScaleFactor;
  const float draw_width = static_cast<float>(tex_size.x) * draw_scale;
  const float draw_height = static_cast<float>(tex_size.y) * draw_scale;
  const float x =
      title_rect_.top_left_x_ + (title_rect_.width_ - draw_width) * 0.5f;
  const float y =
      title_rect_.top_left_y_ + (title_rect_.height_ - draw_height) * 0.5f;
  engine::render::SpriteDrawParams params;
  params.position = {x, y};
  params.scale = {draw_scale, draw_scale};
  renderer.DrawTexture(*title_texture_, params);
}

}  // namespace client
