#include "audio_settings_scene.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/audio/audio_engine.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "ui/menu_background.h"

namespace client {
namespace {

constexpr int kVolumeMin = 0;
constexpr int kVolumeMax = 10;
constexpr float kLabelGap = 2.0f;
constexpr float kValueGap = 10.0f;
constexpr float kTrackMinHeight = 10.0f;
constexpr float kTrackMaxHeight = 18.0f;
constexpr float kTrackWidthScale = 0.24f;
constexpr float kLabelNudgeLeft = 124.0f;
constexpr float kTrackNudgeRight = 124.0f;
constexpr float kResetBackSpacing = 6.0f;
constexpr float kResetPointerExtraWidth = 220.0f;
constexpr float kPointerSpacingBoost = 16.0f;

ui::MenuPointerConfig AudioPointerConfig() {
  return ui::MenuPointerConfig{
      constants::ui::kMenuPointerFramePrefix,
      constants::ui::kMenuPointerFrameExtension,
      constants::ui::OptionsMenu::kPointerFrameCount,
      constants::ui::OptionsMenu::kPointerFrameDuration,
      constants::ui::OptionsMenu::kPointerHeightFactor,
      constants::ui::OptionsMenu::kPointerSpacing + kPointerSpacingBoost,
      constants::ui::OptionsMenu::kPointerScaleFactor};
}

int VolumeFromFloat(float volume) {
  const float clamped = std::max(0.0f, std::min(1.0f, volume));
  return static_cast<int>(std::lround(clamped * kVolumeMax));
}

float VolumeToFloat(int volume) {
  const int clamped = std::max(kVolumeMin, std::min(kVolumeMax, volume));
  return static_cast<float>(clamped) / static_cast<float>(kVolumeMax);
}

void DrawTiledBar(engine::render::Renderer2D& renderer,
                  const std::shared_ptr<engine::render::Texture2D>& mid_texture,
                  const std::shared_ptr<engine::render::Texture2D>& end_texture,
                  const engine::math::RectF& rect) {
  if (!mid_texture || rect.width_ <= 0.0f || rect.height_ <= 0.0f) {
    return;
  }
  const auto mid_size = mid_texture->GetSize();
  if (mid_size.x <= 0 || mid_size.y <= 0) {
    return;
  }

  const float thickness = rect.height_;
  const float mid_scale = thickness / static_cast<float>(mid_size.x);
  if (mid_scale <= 0.0f) {
    return;
  }
  const float mid_segment = static_cast<float>(mid_size.y) * mid_scale;
  if (mid_segment <= 0.0f) {
    return;
  }

  auto draw_segment_ccw =
      [&](const std::shared_ptr<engine::render::Texture2D>& texture, float x,
          float y, float length) {
        if (!texture || length <= 0.0f) {
          return;
        }
        const auto size = texture->GetSize();
        if (size.x <= 0 || size.y <= 0) {
          return;
        }
        const float scale = thickness / static_cast<float>(size.x);
        if (scale <= 0.0f) {
          return;
        }
        const float source_height =
            std::min(static_cast<float>(size.y), length / scale);
        if (source_height <= 0.0f) {
          return;
        }
        const float draw_length = source_height * scale;
        engine::render::SpriteDrawParams params;
        params.position = {x + draw_length, y};
        params.scale = {scale, scale};
        params.rotation = 90.0f;
        params.source = engine::math::RectF{
            0.0f, 0.0f, static_cast<float>(size.x), source_height};
        renderer.DrawTexture(*texture, params);
      };

  auto draw_segment_cw =
      [&](const std::shared_ptr<engine::render::Texture2D>& texture, float x,
          float y, float length, bool mirror) {
        if (!texture || length <= 0.0f) {
          return;
        }
        const auto size = texture->GetSize();
        if (size.x <= 0 || size.y <= 0) {
          return;
        }
        const float scale = thickness / static_cast<float>(size.x);
        if (scale <= 0.0f) {
          return;
        }
        const float source_height =
            std::min(static_cast<float>(size.y), length / scale);
        if (source_height <= 0.0f) {
          return;
        }
        engine::render::SpriteDrawParams params;
        params.position = {x, y + thickness};
        params.scale = {scale, scale};
        params.rotation = -90.0f;
        if (mirror) {
          params.source =
              engine::math::RectF{static_cast<float>(size.x), 0.0f,
                                  -static_cast<float>(size.x), source_height};
        } else {
          params.source = engine::math::RectF{
              0.0f, 0.0f, static_cast<float>(size.x), source_height};
        }
        renderer.DrawTexture(*texture, params);
      };

  float left_cap = 0.0f;
  float right_cap = 0.0f;
  if (end_texture) {
    const auto end_size = end_texture->GetSize();
    if (end_size.x > 0 && end_size.y > 0) {
      const float end_scale = thickness / static_cast<float>(end_size.x);
      const float end_length = static_cast<float>(end_size.y) * end_scale;
      if (end_length * 2.0f < rect.width_) {
        left_cap = end_length;
        right_cap = end_length;
        draw_segment_cw(end_texture, rect.top_left_x_, rect.top_left_y_,
                        end_length, true);
        draw_segment_ccw(end_texture,
                         rect.top_left_x_ + rect.width_ - end_length,
                         rect.top_left_y_, end_length);
      }
    }
  }

  const float mid_start = rect.top_left_x_ + left_cap;
  const float mid_end = rect.top_left_x_ + rect.width_ - right_cap;
  if (mid_end <= mid_start) {
    return;
  }

  float x = mid_start;
  while (x + mid_segment <= mid_end) {
    draw_segment_ccw(mid_texture, x, rect.top_left_y_, mid_segment);
    x += mid_segment;
  }
  const float remaining = mid_end - x;
  if (remaining > 0.0f) {
    draw_segment_ccw(mid_texture, x, rect.top_left_y_, remaining);
  }
}

}  // namespace

AudioSettingsScene::AudioSettingsScene(ClientContext& context)
    : context_(context),
      menu_effects_(context, AudioPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  renderer.SetFont(std::string(constants::ui::kTitleFont));

  menu_effects_.Load();

  for (int i = 0; i < constants::ui::OptionsMenu::kWarningFrameCount; ++i) {
    std::ostringstream path;
    path << constants::ui::OptionsMenu::kWarningFramePrefix << std::setw(4)
         << std::setfill('0') << i
         << constants::ui::OptionsMenu::kWarningFrameExtension;
    auto tex = assets.GetTexture(path.str());
    if (tex) {
      warning_frames_.push_back(tex);
    }
  }

  bar_texture_ = assets.GetTexture(constants::ui::Lobby::kScrollBarMidPath);
  bar_end_texture_ = assets.GetTexture(constants::ui::Lobby::kScrollBarEndPath);
  handle_texture_ =
      assets.GetTexture(constants::ui::Lobby::kScrollBarHandlePath);

  int master_value = kVolumeMax;
  int sfx_value = kVolumeMax;
  int music_value = kVolumeMax;
  if (auto audio = context_.Audio()) {
    master_value = VolumeFromFloat(audio->GetMasterVolume());
    sfx_value = VolumeFromFloat(audio->GetSfxVolume());
    music_value = VolumeFromFloat(audio->GetMusicVolume());
  }

  sliders_.push_back({"Master Volume:", master_value});
  sliders_.push_back({"Sound Volume:", sfx_value});
  sliders_.push_back({"Music Volume:", music_value});

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  for (auto& slider : sliders_) {
    slider.handle_button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{}, "",
        menu_effects_.WrapClick({}));
    slider.handle_button->SetColors(transparent, transparent, transparent);
    slider.handle_button->SetTextColor(white);
    slider.handle_button->SetTextScale(
        constants::ui::OptionsMenu::kButtonTextScale);
    pointer_buttons_.push_back(slider.handle_button);
  }

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  auto padding =
      engine::ui::Insets::Uniform(constants::ui::OptionsMenu::kRootPadding);
  padding.top = constants::ui::OptionsMenu::kRootPaddingTop;
  root->SetPadding(padding);
  root->SetSpacing(constants::ui::OptionsMenu::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_text = std::make_shared<engine::ui::TextElement>(
      "Audio",
      engine::ui::FontSize::Pixels(
          constants::ui::OptionsMenu::kButtonHeight *
          constants::ui::OptionsMenu::kButtonTextScale *
          constants::ui::OptionsMenu::kTitleScaleFactor),
      white);
  title_text->SetFont(std::string(constants::ui::kTitleFont));
  title_text->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text);

  auto warning_slot = std::make_shared<engine::ui::BoxElement>();
  warning_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  warning_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::OptionsMenu::kWarningSlotHeight);
  warning_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  warning_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) { warning_rect_ = rect; });
  root->AddChild(warning_slot);

  auto slider_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  slider_column->SetSpacing(constants::ui::OptionsMenu::kButtonColumnSpacing);
  slider_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  const float slider_slot_height =
      constants::ui::OptionsMenu::kButtonHeight +
      constants::ui::OptionsMenu::kButtonSlotPadding;
  for (auto& slider : sliders_) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(slider_slot_height);
    slot->SetLayoutCallback(
        [&slider](const engine::math::RectF& rect) { slider.row_rect = rect; });
    slider_column->AddChild(slot);
  }

  root->AddChild(slider_column);

  reset_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Reset Defaults", menu_effects_.WrapClick([this]() { ResetDefaults(); }));
  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Back",
      menu_effects_.WrapClick([this]() { context_.OnCloseAudioSettings(); }));
  reset_pointer_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "", menu_effects_.WrapClick({}));

  controls_ = {reset_button_, back_button_};
  pointer_buttons_.push_back(reset_pointer_button_);
  pointer_buttons_.push_back(back_button_);

  const float reset_margin_top =
      std::max(0.0f, constants::ui::OptionsMenu::kBackSlotMarginTop -
                         (slider_slot_height +
                          constants::ui::OptionsMenu::kButtonColumnSpacing));

  auto reset_slot = std::make_shared<engine::ui::BoxElement>();
  reset_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  reset_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(slider_slot_height);
  reset_slot->Layout().margin.top = reset_margin_top;
  reset_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    const float base_width = constants::ui::OptionsMenu::kButtonWidth;
    const float base_height = constants::ui::OptionsMenu::kButtonHeight;
    const float base_x = rect.top_left_x_ + (rect.width_ - base_width) * 0.5f;
    const float base_y =
        rect.top_left_y_ + constants::ui::OptionsMenu::kButtonSlotInset;
    reset_button_->SetPosition({base_x, base_y});
    reset_button_->SetSize({base_width, base_height});

    const float pointer_width = base_width + kResetPointerExtraWidth;
    const float pointer_x =
        rect.top_left_x_ + (rect.width_ - pointer_width) * 0.5f;
    reset_pointer_button_->SetPosition({pointer_x, base_y});
    reset_pointer_button_->SetSize({pointer_width, base_height});
  });
  root->AddChild(reset_slot);

  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  back_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(slider_slot_height);
  back_slot->Layout().margin.top = kResetBackSpacing;
  back_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    back_button_->SetPosition(
        {rect.top_left_x_ +
             (rect.width_ - constants::ui::OptionsMenu::kButtonWidth) * 0.5f,
         rect.top_left_y_ + constants::ui::OptionsMenu::kButtonSlotInset});
    back_button_->SetSize({constants::ui::OptionsMenu::kButtonWidth,
                           constants::ui::OptionsMenu::kButtonHeight});
  });
  root->AddChild(back_slot);

  canvas_.SetRoot(root);

  for (auto& button : controls_) {
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(constants::ui::OptionsMenu::kButtonTextScale);
  }
  reset_pointer_button_->SetColors(transparent, transparent, transparent);
}

void AudioSettingsScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);

  context_.MenuBackground().Update(dt);

  if (!warning_frames_.empty() && warning_animating_) {
    const float max_elapsed = static_cast<float>(warning_frames_.size() - 1) *
                              constants::ui::OptionsMenu::kWarningFrameDuration;
    warning_elapsed_ += dt.as_seconds();
    if (warning_elapsed_ >= max_elapsed) {
      warning_elapsed_ = max_elapsed;
      warning_animating_ = false;
    }
  }

  auto& input = context_.Input();
  const auto mouse_pos = input.GetMousePosition();
  const bool left_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);
  bool sliders_changed = false;

  for (std::size_t i = 0; i < sliders_.size(); ++i) {
    auto& slider = sliders_[i];
    if (!left_down) {
      slider.dragging = false;
    }
    const bool hover_handle = slider.handle_rect.Contains(mouse_pos);
    const bool hover_track = slider.track_rect.Contains(mouse_pos);
    if (left_down && !was_left_down_ && (hover_handle || hover_track)) {
      slider.dragging = true;
    }
    if (slider.dragging && slider.track_rect.width_ > 0.0f) {
      const float raw = (mouse_pos.x - slider.track_rect.top_left_x_) /
                        slider.track_rect.width_;
      const float t = std::max(0.0f, std::min(1.0f, raw));
      const int new_value = std::max(
          kVolumeMin,
          std::min(kVolumeMax, static_cast<int>(std::lround(t * kVolumeMax))));
      if (new_value != slider.value) {
        slider.value = new_value;
        sliders_changed = true;
      }
    }
  }

  if (sliders_changed) {
    ApplyVolumes();
    UpdateSliderLayout(renderer);
  }

  was_left_down_ = left_down;

  menu_effects_.Update(dt, input, pointer_buttons_);
  for (auto& button : controls_) {
    button->Update(dt, input);
  }
}

void AudioSettingsScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void AudioSettingsScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  context_.MenuBackground().Draw(context_.RenderSize());
}

void AudioSettingsScene::DrawForeground(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  DrawWarning(renderer);
  DrawSliders(renderer);
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  reset_button_->Draw(renderer);
  back_button_->Draw(renderer);
  menu_effects_.DrawPointers(renderer, pointer_buttons_);
}

void AudioSettingsScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.Layout(renderer);
  UpdateSliderLayout(renderer);
}

void AudioSettingsScene::DrawWarning(engine::render::Renderer2D& renderer) {
  if (warning_frames_.empty()) {
    return;
  }
  const std::size_t frame_count = warning_frames_.size();
  const float frame_pos =
      warning_elapsed_ / constants::ui::OptionsMenu::kWarningFrameDuration;
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
  const float scale =
      std::min(warning_rect_.width_ / static_cast<float>(tex_size.x),
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

void AudioSettingsScene::DrawSliders(engine::render::Renderer2D& renderer) {
  if (sliders_.empty()) {
    return;
  }

  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const auto white = engine::render::Color::White();
  const float label_font_size = constants::ui::Settings::kVolumeLabelFontSize;

  for (const auto& slider : sliders_) {
    if (slider.row_rect.width_ <= 0.0f || slider.row_rect.height_ <= 0.0f) {
      continue;
    }

    const auto label_size = renderer.MeasureText(slider.label, label_font_size);
    const float label_x = slider.track_rect.top_left_x_ - kTrackNudgeRight -
                          kLabelGap - kLabelNudgeLeft - max_label_width_;
    const float label_y = slider.row_rect.top_left_y_ +
                          (slider.row_rect.height_ - label_size.y) * 0.5f;
    renderer.DrawText(slider.label, {label_x, label_y}, label_font_size, white);

    DrawTiledBar(renderer, bar_texture_, bar_end_texture_, slider.track_rect);

    if (handle_texture_) {
      const auto handle_size = handle_texture_->GetSize();
      if (handle_size.x > 0 && handle_size.y > 0) {
        engine::render::SpriteDrawParams params;
        params.position = {slider.handle_rect.top_left_x_,
                           slider.handle_rect.top_left_y_};
        params.scale = {
            slider.handle_rect.width_ / static_cast<float>(handle_size.x),
            slider.handle_rect.height_ / static_cast<float>(handle_size.y)};
        renderer.DrawTexture(*handle_texture_, params);
      }
    }

    const std::string value_text = std::to_string(slider.value);
    const auto value_size = renderer.MeasureText(value_text, label_font_size);
    const float value_x =
        slider.track_rect.top_left_x_ + slider.track_rect.width_ + kValueGap;
    const float value_y = slider.row_rect.top_left_y_ +
                          (slider.row_rect.height_ - value_size.y) * 0.5f;
    renderer.DrawText(value_text, {value_x, value_y}, label_font_size, white);
  }
}

void AudioSettingsScene::UpdateSliderLayout(
    engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const float label_font_size = constants::ui::Settings::kVolumeLabelFontSize;

  float max_label_width = 0.0f;
  float max_value_width = 0.0f;
  for (auto& slider : sliders_) {
    if (slider.row_rect.width_ <= 0.0f || slider.row_rect.height_ <= 0.0f) {
      slider.label_width = 0.0f;
      slider.value_width = 0.0f;
      continue;
    }

    const auto label_size = renderer.MeasureText(slider.label, label_font_size);
    slider.label_width = label_size.x;
    const std::string value_text = std::to_string(slider.value);
    const auto value_size = renderer.MeasureText(value_text, label_font_size);
    slider.value_width = value_size.x;
    max_label_width = std::max(max_label_width, slider.label_width);
    max_value_width = std::max(max_value_width, slider.value_width);
  }
  max_label_width_ = max_label_width;

  for (auto& slider : sliders_) {
    if (slider.row_rect.width_ <= 0.0f || slider.row_rect.height_ <= 0.0f) {
      continue;
    }

    const float track_height =
        std::max(kTrackMinHeight,
                 std::min(kTrackMaxHeight, slider.row_rect.height_ * 0.3f));
    const float available_width = std::max(
        0.0f, slider.row_rect.width_ - max_label_width - max_value_width -
                  kLabelNudgeLeft - kLabelGap - kTrackNudgeRight - kValueGap);
    const float track_width =
        std::max(0.0f, available_width * kTrackWidthScale);
    const float total_width = max_label_width + kLabelNudgeLeft + kLabelGap +
                              kTrackNudgeRight + track_width + kValueGap +
                              max_value_width;
    const float group_left = slider.row_rect.top_left_x_ +
                             (slider.row_rect.width_ - total_width) * 0.5f;
    const float track_x = group_left + max_label_width + kLabelNudgeLeft +
                          kLabelGap + kTrackNudgeRight;
    const float track_y = slider.row_rect.top_left_y_ +
                          (slider.row_rect.height_ - track_height) * 0.5f;
    slider.track_rect = {track_x, track_y, track_width, track_height};

    float handle_height = constants::ui::Settings::kVolumeButtonSize;
    handle_height = std::min(handle_height, slider.row_rect.height_);
    float handle_width = handle_height;
    if (handle_texture_) {
      const auto handle_size = handle_texture_->GetSize();
      if (handle_size.x > 0 && handle_size.y > 0) {
        const float scale = handle_height / static_cast<float>(handle_size.y);
        handle_width = static_cast<float>(handle_size.x) * scale;
      }
    }

    const float t = VolumeToFloat(slider.value);
    float handle_x = track_x;
    if (track_width > handle_width) {
      handle_x += t * (track_width - handle_width);
    }
    const float handle_y = slider.row_rect.top_left_y_ +
                           (slider.row_rect.height_ - handle_height) * 0.5f;
    slider.handle_rect = {handle_x, handle_y, handle_width, handle_height};

    if (slider.handle_button) {
      const float value_x = track_x + track_width + kValueGap;
      const float pointer_width =
          std::max(0.0f, value_x + slider.value_width - group_left);
      slider.handle_button->SetPosition(
          {group_left, slider.row_rect.top_left_y_});
      slider.handle_button->SetSize({pointer_width, slider.row_rect.height_});
    }
  }
}

void AudioSettingsScene::ApplyVolumes() {
  if (sliders_.size() < 3) {
    return;
  }
  const float master = VolumeToFloat(sliders_[0].value);
  const float sfx = VolumeToFloat(sliders_[1].value);
  const float music = VolumeToFloat(sliders_[2].value);
  context_.SetAudioVolumes(master, music, sfx);
}

void AudioSettingsScene::ResetDefaults() {
  bool changed = false;
  for (auto& slider : sliders_) {
    if (slider.value != kVolumeMax) {
      slider.value = kVolumeMax;
      changed = true;
    }
  }
  if (changed) {
    ApplyVolumes();
    UpdateSliderLayout(context_.Renderer());
  }
}

}  // namespace client
