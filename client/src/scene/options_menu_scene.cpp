#include "options_menu_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "application.h"
#include "audio_paths.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"

namespace client {

namespace {

constexpr float kButtonHeight = 72.0f;
constexpr float kButtonWidth = 170.0f;
constexpr float kButtonTextScale = 0.46f;
constexpr const char* kTitleFont = "title_font";
constexpr float kPointerFrameDuration = 0.06f;
constexpr float kPointerSpacing = 28.0f;
constexpr float kPointerScaleFactor = 0.6f;
constexpr float kTitleScaleFactor = 1.5f;
constexpr float kWarningFrameDuration = 0.06f;
constexpr float kWarningSlotHeight = 50.0f;
constexpr int kWarningFrameCount = 9;
constexpr int kPointerFrameCount = 11;

void PlayUiSound(Application& app, const std::string& path) {
  if (path.empty()) {
    return;
  }
  if (auto audio = app.GetEngine().Audio()) {
    audio->PlaySoundEffect(path);
  }
}

}  // namespace

OptionsMenuScene::OptionsMenuScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont(kTitleFont, "assets/fonts/trajanpro_bold.otf");
  renderer.SetFont(kTitleFont);

  hover_sfx_path_ =
      ResolveAssetPath("assets/song/effects/change_selection.mp3");
  click_sfx_path_ =
      ResolveAssetPath("assets/song/effects/button_confirm.mp3");

  for (int i = 0; i < kPointerFrameCount; ++i) {
    std::ostringstream path;
    path << "assets/ui/main_menu_pointer_anim" << std::setw(4)
         << std::setfill('0') << i << ".png";
    auto tex = renderer.LoadTextureFromFile(path.str());
    if (tex) {
      pointer_frames_.push_back(tex);
    }
  }

  for (int i = 0; i < kWarningFrameCount; ++i) {
    std::ostringstream path;
    path << "assets/ui/Warning_Fleur" << std::setw(4) << std::setfill('0') << i
         << ".png";
    auto tex = renderer.LoadTextureFromFile(path.str());
    if (tex) {
      warning_frames_.push_back(tex);
    }
  }

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  auto padding = engine::ui::Insets::Uniform(48.0f);
  padding.top = 40.0f;
  root->SetPadding(padding);
  root->SetSpacing(14.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_text = std::make_shared<engine::ui::TextElement>(
      "Options", engine::ui::FontSize::Pixels(kButtonHeight *
                                              kButtonTextScale *
                                              kTitleScaleFactor),
      white);
  title_text->SetFont(kTitleFont);
  title_text->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text);

  auto warning_slot = std::make_shared<engine::ui::BoxElement>();
  warning_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  warning_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(kWarningSlotHeight);
  warning_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  warning_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) { warning_rect_ = rect; });
  root->AddChild(warning_slot);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(20.0f);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    buttons_.push_back(button);
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(kButtonHeight + 8.0f);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition(
          {rect.top_left_x_ + (rect.width_ - kButtonWidth) * 0.5f,
           rect.top_left_y_ + 4.0f});
      button->SetSize({kButtonWidth, kButtonHeight});
    });
    button_column->AddChild(slot);
  };

  auto audio_button = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Audio",
      [this]() { PlayUiSound(app_, click_sfx_path_); });
  auto video_button = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Video",
      [this]() { PlayUiSound(app_, click_sfx_path_); });
  auto keyboard_button = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Keyboard",
      [this]() { PlayUiSound(app_, click_sfx_path_); });
  auto back_button = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Back",
      [this]() {
        PlayUiSound(app_, click_sfx_path_);
        app_.OnCloseSettings();
      });

  add_slot(audio_button);
  add_slot(video_button);
  add_slot(keyboard_button);

  root->AddChild(button_column);

  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  back_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(kButtonHeight + 8.0f);
  back_slot->Layout().margin.top = 116.0f;
  back_slot->SetLayoutCallback([back_button](const engine::math::RectF& rect) {
    back_button->SetPosition(
        {rect.top_left_x_ + (rect.width_ - kButtonWidth) * 0.5f,
         rect.top_left_y_ + 4.0f});
    back_button->SetSize({kButtonWidth, kButtonHeight});
  });
  root->AddChild(back_slot);

  buttons_.push_back(back_button);

  canvas_.SetRoot(root);

  for (auto& button : buttons_) {
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(kButtonTextScale);
  }
}

void OptionsMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);

  if (!warning_frames_.empty() && warning_animating_) {
    const float max_elapsed =
        static_cast<float>(warning_frames_.size() - 1) *
        kWarningFrameDuration;
    warning_elapsed_ += dt.as_seconds();
    if (warning_elapsed_ >= max_elapsed) {
      warning_elapsed_ = max_elapsed;
      warning_animating_ = false;
    }
  }

  auto& input = app_.GetEngine().Input();
  const auto mouse_pos = input.GetMousePosition();
  const float max_elapsed =
      pointer_frames_.empty()
          ? 0.0f
          : (static_cast<float>(pointer_frames_.size() - 1) *
             kPointerFrameDuration);
  for (std::size_t i = 0; i < buttons_.size(); ++i) {
    auto& button = buttons_[i];
    if (!button) {
      continue;
    }
    auto& state = pointer_states_[i];
    state.was_hovered = state.hovered;
    state.hovered = engine::math::RectF{button->GetPosition(),
                                        button->GetSize()}
                        .Contains(mouse_pos);
    if (state.hovered && !state.was_hovered) {
      state.elapsed = 0.0f;
      state.animating = true;
      PlayUiSound(app_, hover_sfx_path_);
    }
    if (state.hovered && state.animating && max_elapsed > 0.0f) {
      state.elapsed += dt.as_seconds();
      if (state.elapsed >= max_elapsed) {
        state.elapsed = max_elapsed;
        state.animating = false;
      }
    }
    if (!state.hovered) {
      state.animating = false;
      state.elapsed = 0.0f;
    }
  }

  for (auto& button : buttons_) {
    button->Update(dt, input);
  }
}

void OptionsMenuScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.SetFont(kTitleFont);
  LayoutUi(renderer);

  canvas_.Draw(renderer);
  DrawWarning(renderer);
  renderer.SetFont(kTitleFont);
  for (auto& button : buttons_) {
    button->Draw(renderer);
  }
  DrawPointers(renderer);
}

void OptionsMenuScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

void OptionsMenuScene::DrawPointers(engine::render::Renderer2D& renderer) {
  if (pointer_frames_.empty()) {
    return;
  }
  const std::size_t frame_count = pointer_frames_.size();
  for (std::size_t i = 0; i < buttons_.size(); ++i) {
    auto& button = buttons_[i];
    const auto& state = pointer_states_[i];
    if (!button || !state.hovered) {
      continue;
    }

    const float frame_pos = state.elapsed / kPointerFrameDuration;
    const std::size_t frame_index =
        std::min(frame_count - 1, static_cast<std::size_t>(frame_pos));
    auto texture = pointer_frames_[frame_index];
    if (!texture) {
      continue;
    }
    const auto tex_size = texture->GetSize();
    if (tex_size.y == 0) {
      continue;
    }

    const float scale =
        (kButtonHeight * 0.85f * kPointerScaleFactor) /
        static_cast<float>(tex_size.y);
    const float scaled_width = static_cast<float>(tex_size.x) * scale;
    const float scaled_height = static_cast<float>(tex_size.y) * scale;
    const auto pos = button->GetPosition();
    const auto size = button->GetSize();
    const float y = pos.y + (size.y - scaled_height) * 0.5f;
    const float left_x = pos.x - scaled_width - kPointerSpacing;
    const float right_x = pos.x + size.x + kPointerSpacing;

    engine::render::SpriteDrawParams left_params;
    left_params.position = {left_x, y};
    left_params.scale = {scale, scale};
    renderer.DrawTexture(*texture, left_params);

    engine::render::SpriteDrawParams right_params;
    right_params.position = {right_x, y};
    right_params.scale = {scale, scale};
    right_params.source = engine::math::RectF{
        static_cast<float>(tex_size.x), 0.0f,
        -static_cast<float>(tex_size.x), static_cast<float>(tex_size.y)};
    renderer.DrawTexture(*texture, right_params);
  }
}

void OptionsMenuScene::DrawWarning(engine::render::Renderer2D& renderer) {
  if (warning_frames_.empty()) {
    return;
  }
  const std::size_t frame_count = warning_frames_.size();
  const float frame_pos = warning_elapsed_ / kWarningFrameDuration;
  const std::size_t frame_index =
      std::min(frame_count - 1, static_cast<std::size_t>(frame_pos));
  auto texture = warning_frames_[frame_index];
  if (!texture) {
    return;
  }
  const auto tex_size = texture->GetSize();
  if (tex_size.y == 0) {
    return;
  }
  const float scale = std::min(
      warning_rect_.width_ / static_cast<float>(tex_size.x),
      warning_rect_.height_ / static_cast<float>(tex_size.y));
  if (scale <= 0.0f) {
    return;
  }
  const float draw_width = static_cast<float>(tex_size.x) * scale;
  const float draw_height = static_cast<float>(tex_size.y) * scale;
  const float x =
      warning_rect_.top_left_x_ + (warning_rect_.width_ - draw_width) * 0.5f;
  const float y =
      warning_rect_.top_left_y_ + (warning_rect_.height_ - draw_height) * 0.5f;
  engine::render::SpriteDrawParams params;
  params.position = {x, y};
  params.scale = {scale, scale};
  renderer.DrawTexture(*texture, params);
}

}  // namespace client
