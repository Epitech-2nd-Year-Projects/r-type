#include "pause_scene.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace client {
namespace {

ui::MenuPointerConfig PausePointerConfig() {
  return ui::MenuPointerConfig{constants::ui::kMenuPointerFramePrefix,
                               constants::ui::kMenuPointerFrameExtension,
                               constants::ui::MainMenu::kPointerFrameCount,
                               constants::ui::MainMenu::kPointerFrameDuration,
                               constants::ui::MainMenu::kPointerHeightFactor,
                               constants::ui::MainMenu::kPointerSpacing +
                                   constants::ui::Pause::kPointerSpacingExtra,
                               constants::ui::MainMenu::kPointerScaleFactor};
}

void LoadFleurFrames(
    ClientAssetManager& assets,
    std::vector<std::shared_ptr<engine::render::Texture2D>>& frames,
    std::string_view prefix) {
  frames.clear();
  frames.reserve(
      static_cast<std::size_t>(constants::ui::Pause::kFleurFrameCount));
  for (int i = 0; i < constants::ui::Pause::kFleurFrameCount; ++i) {
    std::ostringstream path;
    path << prefix << std::setw(4) << std::setfill('0') << i
         << constants::ui::Pause::kFleurFrameExtension;
    auto texture = assets.GetTexture(path.str());
    if (texture) {
      frames.push_back(texture);
    }
  }
}

void AdvanceFleur(
    float dt_seconds,
    const std::vector<std::shared_ptr<engine::render::Texture2D>>& frames,
    float& elapsed) {
  if (frames.empty() || constants::ui::Pause::kFleurFrameDuration <= 0.0f) {
    return;
  }
  const float max_elapsed = static_cast<float>(frames.size() - 1) *
                            constants::ui::Pause::kFleurFrameDuration;
  if (max_elapsed <= 0.0f) {
    return;
  }
  if (elapsed >= max_elapsed) {
    return;
  }
  elapsed = std::min(elapsed + dt_seconds, max_elapsed);
}

void DrawFleur(
    engine::render::Renderer2D& renderer,
    const std::vector<std::shared_ptr<engine::render::Texture2D>>& frames,
    const engine::math::RectF& rect, float elapsed) {
  if (frames.empty() || constants::ui::Pause::kFleurFrameDuration <= 0.0f) {
    return;
  }
  const std::size_t frame_index =
      std::min(frames.size() - 1,
               static_cast<std::size_t>(
                   elapsed / constants::ui::Pause::kFleurFrameDuration));
  auto texture = frames[frame_index];
  if (!texture) {
    return;
  }
  const auto tex_size = texture->GetSize();
  if (tex_size.y == 0 || rect.width_ <= 0.0f || rect.height_ <= 0.0f) {
    return;
  }
  const float scale = std::min(rect.width_ / static_cast<float>(tex_size.x),
                               rect.height_ / static_cast<float>(tex_size.y));
  if (scale <= 0.0f) {
    return;
  }
  const float draw_width = static_cast<float>(tex_size.x) * scale;
  const float draw_height = static_cast<float>(tex_size.y) * scale;
  const float x = rect.top_left_x_ + (rect.width_ - draw_width) * 0.5f;
  const float y = rect.top_left_y_ + (rect.height_ - draw_height) * 0.5f;
  engine::render::SpriteDrawParams params;
  params.position = {x, y};
  params.scale = {scale, scale};
  renderer.DrawTexture(*texture, params);
}

}  // namespace

PauseScene::PauseScene(ClientContext& context)
    : context_(context),
      menu_effects_(context, PausePointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  menu_effects_.Load();

  LoadFleurFrames(assets, top_frames_,
                  constants::ui::Pause::kTopFleurFramePrefix);
  LoadFleurFrames(assets, bottom_frames_,
                  constants::ui::Pause::kBottomFleurFramePrefix);

  auto& input = context_.Input();
  pause_toggle_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
  confirm_pressed_ =
      input.IsActionActive(std::string(constants::input::kActionConfirm));

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  resume_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Continue",
      menu_effects_.WrapClick([this]() { context_.OnGameResume(); }));
  options_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Options",
      menu_effects_.WrapClick([this]() { context_.OnOpenSettings(); }));
  quit_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Quit to Menu",
      menu_effects_.WrapClick([this]() { context_.OnQuitToMenu(); }));

  menu_buttons_.push_back(resume_button_);
  menu_buttons_.push_back(options_button_);
  menu_buttons_.push_back(quit_button_);

  for (auto& button : menu_buttons_) {
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
  }

  BuildUi();
}

void PauseScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();

  menu_effects_.Update(dt, input, menu_buttons_);
  for (auto& button : menu_buttons_) {
    button->Update(dt, input);
  }

  const float dt_seconds = dt.as_seconds();
  AdvanceFleur(dt_seconds, top_frames_, top_elapsed_);
  AdvanceFleur(dt_seconds, bottom_frames_, bottom_elapsed_);

  const bool pause_toggle =
      input.IsActionActive(std::string(constants::input::kActionPause)) ||
      input.IsActionActive(std::string(constants::input::kActionCancel));
  if (pause_toggle && !pause_toggle_pressed_) {
    context_.OnGameResume();
  }
  pause_toggle_pressed_ = pause_toggle;

  const bool confirm_pressed =
      input.IsActionActive(std::string(constants::input::kActionConfirm));
  if (confirm_pressed && !confirm_pressed_) {
    context_.OnGameResume();
  }
  confirm_pressed_ = confirm_pressed;
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  renderer.DrawRect({0.0f, 0.0f, static_cast<float>(render_size.x),
                     static_cast<float>(render_size.y)},
                    constants::ui::Pause::kOverlayColor);

  canvas_.Draw(renderer);
  DrawFleur(renderer, top_frames_, top_rect_, top_elapsed_);
  DrawFleur(renderer, bottom_frames_, bottom_rect_, bottom_elapsed_);
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  for (auto& button : menu_buttons_) {
    button->Draw(renderer);
  }
  menu_effects_.DrawPointers(renderer, menu_buttons_);
  renderer.SetFont(std::string(constants::ui::kBodyFont));
}

void PauseScene::BuildUi() {
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(
      engine::ui::Insets::Uniform(constants::ui::Pause::kRootPadding));
  root->SetSpacing(constants::ui::Pause::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto list_stack =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  list_stack->Layout().size.width =
      engine::ui::LayoutValue::Pixels(constants::ui::Pause::kListWidth);
  list_stack->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  list_stack->SetSpacing(constants::ui::Pause::kDecorationSpacing);

  auto top_slot = std::make_shared<engine::ui::BoxElement>();
  top_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  top_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Pause::kTopFleurSlotHeight);
  top_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  top_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) { top_rect_ = rect; });
  list_stack->AddChild(top_slot);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(constants::ui::MainMenu::kButtonColumnSpacing);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<engine::ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
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

  add_slot(resume_button_);
  add_slot(options_button_);
  add_slot(quit_button_);

  list_stack->AddChild(button_column);

  auto bottom_slot = std::make_shared<engine::ui::BoxElement>();
  bottom_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  bottom_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Pause::kBottomFleurSlotHeight);
  bottom_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  bottom_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) { bottom_rect_ = rect; });
  list_stack->AddChild(bottom_slot);

  root->AddChild(list_stack);

  canvas_.SetRoot(root);
}

void PauseScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
