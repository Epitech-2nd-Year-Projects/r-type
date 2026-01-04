#include "scene/splash_scene.h"

#include <algorithm>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/audio/audio_engine.h"
#include "engine/input.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {
namespace {

bool IsAnyKeyDown(engine::input::InputManager& input) {
  for (int key = static_cast<int>(engine::input::Key::kA);
       key <= static_cast<int>(engine::input::Key::kF4); ++key) {
    if (input.IsKeyDown(static_cast<engine::input::Key>(key))) {
      return true;
    }
  }
  return false;
}

bool IsAnyMouseButtonDown(engine::input::InputManager& input) {
  for (int button = static_cast<int>(engine::input::MouseButton::kLeft);
       button <= static_cast<int>(engine::input::MouseButton::kButton5);
       ++button) {
    if (input.IsMouseButtonDown(
            static_cast<engine::input::MouseButton>(button))) {
      return true;
    }
  }
  return false;
}

}  // namespace

SplashScene::SplashScene(ClientContext& context)
    : context_(context),
      background_(constants::ui::SplashScreen::kBackgroundVideoPath),
      prompt_text_(constants::ui::SplashScreen::kPromptText),
      copyright_text_(constants::ui::SplashScreen::kCopyrightText) {
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  transition_sfx_path_ =
      assets.GetSfxPath(constants::ui::SplashScreen::kTransitionSfxPath);
  title_texture_ =
      assets.GetTexture(constants::ui::MainMenu::kTitleTexturePath);
}

void SplashScene::Update(engine::time::TimeDelta dt) {
  context_.MenuBackground().Update(dt);
  background_.Update(dt);
  auto& input = context_.Input();
  const bool input_down = IsAnyInputDown(input);
  if (!transition_started_ && input_down && !input_was_down_) {
    TriggerTransition();
  }
  if (transition_started_) {
    transition_elapsed_ += dt;
    const auto duration = engine::time::TimeDelta::from_seconds(
        constants::ui::SplashScreen::kTransitionFadeSeconds);
    if (duration <= engine::time::TimeDelta::zero() ||
        transition_elapsed_ >= duration) {
      context_.OnQuitToMenu();
    }
  }
  input_was_down_ = input_down;
}

void SplashScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void SplashScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  background_.Draw(context_.Window());
  const float transition_alpha = TransitionAlpha();
  if (transition_alpha > 0.0f) {
    context_.MenuBackground().Draw(context_.Window(), transition_alpha);
  }
}

void SplashScene::DrawForeground(engine::render::Renderer2D& renderer) {
  DrawLogo(renderer);
  DrawPrompt(renderer);
  DrawCopyright(renderer);
}

engine::math::RectF SplashScene::ComputeTitleRect(float width,
                                                  float height) const {
  const float padding = constants::ui::MainMenu::kRootPadding;
  const float available_width = std::max(0.0f, width - padding * 2.0f);
  const float available_height = std::max(0.0f, height - padding * 2.0f);
  const float button_slot_height = constants::ui::MainMenu::kButtonHeight +
                                   constants::ui::MainMenu::kButtonSlotPadding;
  const int button_count = constants::ui::MainMenu::kButtonCount;
  const float button_column_height =
      static_cast<float>(button_count) * button_slot_height +
      static_cast<float>(std::max(0, button_count - 1)) *
          constants::ui::MainMenu::kButtonColumnSpacing;
  const float content_height = constants::ui::MainMenu::kTitleSlotHeight +
                               constants::ui::MainMenu::kRootSpacing +
                               button_column_height;
  const float start_y = padding + (available_height - content_height) * 0.5f;
  return {padding, start_y + constants::ui::MainMenu::kTitleYOffset,
          available_width, constants::ui::MainMenu::kTitleSlotHeight};
}

void SplashScene::DrawLogo(engine::render::Renderer2D& renderer) {
  if (!title_texture_) {
    return;
  }
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);
  if (width <= 0.0f || height <= 0.0f) {
    return;
  }
  title_rect_ = ComputeTitleRect(width, height);
  if (title_rect_.width_ <= 0.0f || title_rect_.height_ <= 0.0f) {
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

void SplashScene::DrawPrompt(engine::render::Renderer2D& renderer) {
  if (prompt_text_.empty()) {
    return;
  }
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);
  if (width <= 0.0f || height <= 0.0f) {
    return;
  }
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const float font_size =
      std::min(width, height) * constants::ui::SplashScreen::kPromptFontScale;
  if (font_size <= 0.0f) {
    return;
  }
  const auto text_size = renderer.MeasureText(prompt_text_, font_size);
  const float x = (width - text_size.x) * 0.5f;
  const float padding =
      height * constants::ui::SplashScreen::kPromptBottomPaddingRatio;
  const float y = height - padding - text_size.y;
  renderer.DrawText(prompt_text_, {x, y}, font_size,
                    constants::ui::SplashScreen::kPromptColor);
}

void SplashScene::DrawCopyright(engine::render::Renderer2D& renderer) {
  if (copyright_text_.empty()) {
    return;
  }
  const auto window_size = context_.Window().GetSize();
  const float width = static_cast<float>(window_size.x);
  const float height = static_cast<float>(window_size.y);
  if (width <= 0.0f || height <= 0.0f) {
    return;
  }
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  const float font_size = std::min(width, height) *
                          constants::ui::SplashScreen::kCopyrightFontScale;
  if (font_size <= 0.0f) {
    return;
  }
  const auto text_size = renderer.MeasureText(copyright_text_, font_size);
  const float x = (width - text_size.x) * 0.5f;
  const float padding =
      height * constants::ui::SplashScreen::kCopyrightBottomPaddingRatio;
  const float y = height - padding - text_size.y;
  renderer.DrawText(copyright_text_, {x, y}, font_size,
                    constants::ui::SplashScreen::kCopyrightColor);
}

float SplashScene::TransitionAlpha() const {
  if (!transition_started_) {
    return 0.0f;
  }
  const float duration = constants::ui::SplashScreen::kTransitionFadeSeconds;
  if (duration <= 0.0f) {
    return 1.0f;
  }
  const float progress = transition_elapsed_.as_seconds() / duration;
  if (progress <= 0.0f) {
    return 0.0f;
  }
  if (progress >= 1.0f) {
    return 1.0f;
  }
  return progress;
}

bool SplashScene::IsAnyInputDown(engine::input::InputManager& input) const {
  return IsAnyKeyDown(input) || IsAnyMouseButtonDown(input);
}

void SplashScene::TriggerTransition() {
  if (transition_started_) {
    return;
  }
  if (constants::ui::SplashScreen::kTransitionFadeSeconds <= 0.0f) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Splash transition fade seconds must be positive");
  }
  transition_started_ = true;
  transition_elapsed_ = engine::time::TimeDelta::zero();
  context_.Input().ClearState();
  if (!transition_sfx_path_.empty()) {
    if (auto audio = context_.Audio()) {
      audio->PlaySoundEffect(transition_sfx_path_);
    }
  }
}

}  // namespace client
