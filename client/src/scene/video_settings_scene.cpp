#include "video_settings_scene.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include "client_asset_manager.h"
#include "client_config.h"
#include "client_context.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "ui/menu_background.h"

namespace client {
namespace {

constexpr float kRowButtonWidth = 520.0f;
constexpr float kRowTextPadding = 24.0f;
constexpr float kBottomSpacing = 6.0f;
constexpr float kResetPointerExtraWidth = 220.0f;
constexpr float kPointerSpacingBoost = 16.0f;

ui::MenuPointerConfig VideoPointerConfig() {
  return ui::MenuPointerConfig{
      constants::ui::kMenuPointerFramePrefix,
      constants::ui::kMenuPointerFrameExtension,
      constants::ui::OptionsMenu::kPointerFrameCount,
      constants::ui::OptionsMenu::kPointerFrameDuration,
      constants::ui::OptionsMenu::kPointerHeightFactor,
      constants::ui::OptionsMenu::kPointerSpacing + kPointerSpacingBoost,
      constants::ui::OptionsMenu::kPointerScaleFactor};
}

}  // namespace

VideoSettingsScene::VideoSettingsScene(ClientContext& context)
    : context_(context),
      menu_effects_(context, VideoPointerConfig(),
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

  resolutions_ = {{1280, 720, "1280x720"},
                  {1600, 900, "1600x900"},
                  {1920, 1080, "1920x1080"},
                  {2560, 1440, "2560x1440"}};
  fps_options_ = {30, 60, 120, 144, 240, 0};

  std::vector<std::string> resolution_labels;
  resolution_labels.reserve(resolutions_.size());
  for (const auto& option : resolutions_) {
    resolution_labels.push_back(option.label);
  }

  std::vector<std::string> fps_labels;
  fps_labels.reserve(fps_options_.size());
  for (int fps : fps_options_) {
    if (fps <= 0) {
      fps_labels.emplace_back("Unlimited");
    } else {
      fps_labels.push_back(std::to_string(fps));
    }
  }

  const std::vector<std::string> on_off{"Off", "On"};
  rows_.push_back({SettingId::kResolution, "Resolution:", resolution_labels});
  rows_.push_back({SettingId::kFullscreen, "Full Screen:", on_off});
  rows_.push_back({SettingId::kVsync, "V-Sync:", on_off});
  rows_.push_back({SettingId::kMaxFps, "Max FPS:", fps_labels});

  auto& runtime_config = context_.Config();
  const ClientConfig defaults{};
  pending_resolution_width_ = runtime_config.GetInt(
      std::string(constants::config::kVideoResolutionWidth),
      defaults.resolution_width);
  pending_resolution_height_ = runtime_config.GetInt(
      std::string(constants::config::kVideoResolutionHeight),
      defaults.resolution_height);
  pending_fullscreen_ = runtime_config.GetBool(
      std::string(constants::config::kVideoFullscreen), defaults.fullscreen);
  pending_vsync_ = runtime_config.GetBool(
      std::string(constants::config::kVideoVsync), defaults.vsync);
  const int stored_target_fps = runtime_config.GetInt(
      std::string(constants::config::kVideoTargetFps), defaults.target_fps);
  pending_target_fps_ = std::max(0, stored_target_fps);

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  for (auto& row : rows_) {
    const SettingId row_id = row.id;
    row.button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{}, "",
        menu_effects_.WrapClick([this, row_id]() { AdvanceSetting(row_id); }));
    row.button->SetColors(transparent, transparent, transparent);
    row.button->SetTextColor(white);
    row.button->SetTextScale(constants::ui::OptionsMenu::kButtonTextScale);
    controls_.push_back(row.button);
    pointer_buttons_.push_back(row.button);
  }

  apply_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Apply", menu_effects_.WrapClick([this]() { ApplySettings(); }));
  reset_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Reset Defaults", menu_effects_.WrapClick([this]() { ResetDefaults(); }));
  reset_pointer_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "", menu_effects_.WrapClick({}));
  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Back",
      menu_effects_.WrapClick([this]() { context_.OnCloseVideoSettings(); }));

  controls_.push_back(apply_button_);
  controls_.push_back(reset_button_);
  controls_.push_back(back_button_);
  pointer_buttons_.push_back(apply_button_);
  pointer_buttons_.push_back(reset_pointer_button_);
  pointer_buttons_.push_back(back_button_);

  for (auto& button : controls_) {
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(constants::ui::OptionsMenu::kButtonTextScale);
  }
  reset_pointer_button_->SetColors(transparent, transparent, transparent);

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
      "Video",
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

  auto row_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  row_column->SetSpacing(constants::ui::OptionsMenu::kButtonColumnSpacing);
  row_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  const float row_slot_height = constants::ui::OptionsMenu::kButtonHeight +
                                constants::ui::OptionsMenu::kButtonSlotPadding;
  for (std::size_t i = 0; i < rows_.size(); ++i) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(row_slot_height);
    slot->SetLayoutCallback([this, i](const engine::math::RectF& rect) {
      auto& row = rows_[i];
      row.row_rect = rect;
      const float base_width = kRowButtonWidth;
      const float base_height = constants::ui::OptionsMenu::kButtonHeight;
      const float base_x = rect.top_left_x_ + (rect.width_ - base_width) * 0.5f;
      const float base_y =
          rect.top_left_y_ + constants::ui::OptionsMenu::kButtonSlotInset;
      row.button->SetPosition({base_x, base_y});
      row.button->SetSize({base_width, base_height});
    });
    row_column->AddChild(slot);
  }

  root->AddChild(row_column);

  const float action_margin_top = std::max(
      0.0f,
      constants::ui::OptionsMenu::kBackSlotMarginTop -
          (row_slot_height + constants::ui::OptionsMenu::kButtonColumnSpacing));

  auto apply_slot = std::make_shared<engine::ui::BoxElement>();
  apply_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  apply_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(row_slot_height);
  apply_slot->Layout().margin.top = action_margin_top;
  apply_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    apply_button_->SetPosition(
        {rect.top_left_x_ +
             (rect.width_ - constants::ui::OptionsMenu::kButtonWidth) * 0.5f,
         rect.top_left_y_ + constants::ui::OptionsMenu::kButtonSlotInset});
    apply_button_->SetSize({constants::ui::OptionsMenu::kButtonWidth,
                            constants::ui::OptionsMenu::kButtonHeight});
  });
  root->AddChild(apply_slot);

  auto reset_slot = std::make_shared<engine::ui::BoxElement>();
  reset_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  reset_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(row_slot_height);
  reset_slot->Layout().margin.top = kBottomSpacing;
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
      engine::ui::LayoutValue::Pixels(row_slot_height);
  back_slot->Layout().margin.top = kBottomSpacing;
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
  SyncRowsFromState();
}

void VideoSettingsScene::Update(engine::time::TimeDelta dt) {
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
  menu_effects_.Update(dt, input, pointer_buttons_);
  for (auto& button : controls_) {
    button->Update(dt, input);
  }
}

void VideoSettingsScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void VideoSettingsScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  context_.MenuBackground().Draw(context_.RenderSize());
}

void VideoSettingsScene::DrawForeground(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  DrawWarning(renderer);
  DrawRows(renderer);
  for (auto& button : controls_) {
    button->Draw(renderer);
  }
  menu_effects_.DrawPointers(renderer, pointer_buttons_);
}

void VideoSettingsScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.Layout(renderer);
}

void VideoSettingsScene::DrawWarning(engine::render::Renderer2D& renderer) {
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

void VideoSettingsScene::DrawRows(engine::render::Renderer2D& renderer) {
  if (rows_.empty()) {
    return;
  }
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const auto white = engine::render::Color::White();
  const float font_size = constants::ui::Settings::kVolumeLabelFontSize;

  for (const auto& row : rows_) {
    if (!row.button || row.options.empty()) {
      continue;
    }
    const auto pos = row.button->GetPosition();
    const auto size = row.button->GetSize();
    if (size.x <= 0.0f || size.y <= 0.0f) {
      continue;
    }
    const std::string value_text =
        (row.index >= 0 &&
         static_cast<std::size_t>(row.index) < row.options.size())
            ? row.options[row.index]
            : std::string{};
    const auto label_size = renderer.MeasureText(row.label, font_size);
    const auto value_size = renderer.MeasureText(value_text, font_size);
    const float label_x = pos.x + kRowTextPadding;
    const float label_y = pos.y + (size.y - label_size.y) * 0.5f;
    const float value_x = pos.x + size.x - kRowTextPadding - value_size.x;
    const float value_y = pos.y + (size.y - value_size.y) * 0.5f;
    renderer.DrawText(row.label, {label_x, label_y}, font_size, white);
    renderer.DrawText(value_text, {value_x, value_y}, font_size, white);
  }
}

void VideoSettingsScene::AdvanceSetting(SettingId id) {
  if (resolutions_.empty() || fps_options_.empty()) {
    return;
  }

  switch (id) {
    case SettingId::kResolution: {
      int index = FindResolutionIndex(pending_resolution_width_,
                                      pending_resolution_height_);
      index = (index + 1) % static_cast<int>(resolutions_.size());
      pending_resolution_width_ = resolutions_[index].width;
      pending_resolution_height_ = resolutions_[index].height;
      break;
    }
    case SettingId::kFullscreen:
      pending_fullscreen_ = !pending_fullscreen_;
      break;
    case SettingId::kVsync:
      pending_vsync_ = !pending_vsync_;
      break;
    case SettingId::kMaxFps: {
      int index = FindFpsIndex(pending_target_fps_);
      index = (index + 1) % static_cast<int>(fps_options_.size());
      pending_target_fps_ = fps_options_[index];
      break;
    }
  }
  SyncRowsFromState();
}

void VideoSettingsScene::SyncRowsFromState() {
  for (auto& row : rows_) {
    switch (row.id) {
      case SettingId::kResolution: {
        int index = FindResolutionIndex(pending_resolution_width_,
                                        pending_resolution_height_);
        row.index = index;
        pending_resolution_width_ = resolutions_[index].width;
        pending_resolution_height_ = resolutions_[index].height;
        break;
      }
      case SettingId::kFullscreen:
        row.index = pending_fullscreen_ ? 1 : 0;
        break;
      case SettingId::kVsync:
        row.index = pending_vsync_ ? 1 : 0;
        break;
      case SettingId::kMaxFps: {
        if (fps_options_.empty()) {
          row.index = 0;
          break;
        }
        int index = FindFpsIndex(pending_target_fps_);
        row.index = index;
        pending_target_fps_ = fps_options_[index];
        break;
      }
    }
  }
}

void VideoSettingsScene::ApplySettings() {
  context_.SetVideoSettings(pending_resolution_width_,
                            pending_resolution_height_, pending_fullscreen_,
                            pending_vsync_, pending_target_fps_);
}

void VideoSettingsScene::ResetDefaults() {
  const ClientConfig defaults{};
  pending_resolution_width_ = defaults.resolution_width;
  pending_resolution_height_ = defaults.resolution_height;
  pending_fullscreen_ = defaults.fullscreen;
  pending_vsync_ = defaults.vsync;
  pending_target_fps_ = defaults.target_fps;
  SyncRowsFromState();
}

int VideoSettingsScene::FindResolutionIndex(int width, int height) const {
  for (std::size_t i = 0; i < resolutions_.size(); ++i) {
    if (resolutions_[i].width == width && resolutions_[i].height == height) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

int VideoSettingsScene::FindFpsIndex(int target_fps) const {
  if (fps_options_.empty()) {
    return 0;
  }
  if (target_fps <= 0) {
    for (std::size_t i = 0; i < fps_options_.size(); ++i) {
      if (fps_options_[i] <= 0) {
        return static_cast<int>(i);
      }
    }
    return 0;
  }
  int best_index = 0;
  int best_delta = std::abs(fps_options_.front() - target_fps);
  for (std::size_t i = 1; i < fps_options_.size(); ++i) {
    const int delta = std::abs(fps_options_[i] - target_fps);
    if (delta < best_delta) {
      best_delta = delta;
      best_index = static_cast<int>(i);
    }
  }
  return best_index;
}

}  // namespace client
