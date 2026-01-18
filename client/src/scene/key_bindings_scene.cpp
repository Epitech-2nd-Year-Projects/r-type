#include "key_bindings_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "client_asset_manager.h"
#include "client_context.h"
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
constexpr float kPointerSpacingBoost = 16.0f;

ui::MenuPointerConfig BindingsPointerConfig() {
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

KeyBindingsScene::KeyBindingsScene(ClientContext& context)
    : context_(context),
      menu_effects_(context, BindingsPointerConfig(),
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

  const auto& bindings = context_.KeyBindingSet();
  const auto actions = bindings.Actions();

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  for (const auto action : actions) {
    auto button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{}, "",
        menu_effects_.WrapClick([this, action]() { HandleRebind(action); }));
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(constants::ui::OptionsMenu::kButtonTextScale);

    rows_.push_back({action, ActionLabel(action), button});
    controls_.push_back(button);
    pointer_buttons_.push_back(button);
  }

  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::OptionsMenu::kButtonWidth,
                             constants::ui::OptionsMenu::kButtonHeight},
      "Back",
      menu_effects_.WrapClick([this]() { context_.OnCloseKeyBindings(); }));

  controls_.push_back(back_button_);
  pointer_buttons_.push_back(back_button_);

  back_button_->SetColors(transparent, transparent, transparent);
  back_button_->SetTextColor(white);
  back_button_->SetTextScale(constants::ui::OptionsMenu::kButtonTextScale);

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
      "Keyboard",
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

  auto bindings_container = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kHorizontal);
  bindings_container->SetSpacing(
      constants::ui::Settings::kBindingColumnSpacing * 15.0f);
  bindings_container->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto left_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  left_column->SetSpacing(constants::ui::OptionsMenu::kButtonColumnSpacing);

  auto right_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  right_column->SetSpacing(constants::ui::OptionsMenu::kButtonColumnSpacing);

  const float row_slot_height = constants::ui::OptionsMenu::kButtonHeight +
                                constants::ui::OptionsMenu::kButtonSlotPadding;

  const std::size_t half_count = (rows_.size() + 1) / 2;

  for (std::size_t i = 0; i < rows_.size(); ++i) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(row_slot_height);
    slot->Layout().size.width =
        engine::ui::LayoutValue::Pixels(kRowButtonWidth);

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

    if (i < half_count) {
      left_column->AddChild(slot);
    } else {
      right_column->AddChild(slot);
    }
  }

  bindings_container->AddChild(left_column);
  bindings_container->AddChild(right_column);
  root->AddChild(bindings_container);

  const float action_margin_top = constants::ui::Settings::kBackMarginTop;

  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  back_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(row_slot_height);
  back_slot->Layout().margin.top = action_margin_top;
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
}

void KeyBindingsScene::Update(engine::time::TimeDelta dt) {
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

  if (is_binding_) {
    auto bindable_keys = BindableKeys();
    for (const auto key : bindable_keys) {
      if (input.IsKeyDown(key)) {
        if (key == engine::input::Key::kEscape) {
          is_binding_ = false;
        } else {
          auto result = context_.UpdateKeyBinding(binding_action_, key);
          is_binding_ = false;
        }
        break;
      }
    }
  } else {
    menu_effects_.Update(dt, input, pointer_buttons_);
    for (auto& button : controls_) {
      button->Update(dt, input);
    }
  }
}

void KeyBindingsScene::Draw(engine::render::Renderer2D& renderer) {
  DrawBackground(renderer);
  DrawForeground(renderer);
}

void KeyBindingsScene::DrawBackground(engine::render::Renderer2D& renderer) {
  static_cast<void>(renderer);
  context_.MenuBackground().Draw(context_.RenderSize());
}

void KeyBindingsScene::DrawForeground(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  DrawWarning(renderer);
  DrawRows(renderer);

  if (!is_binding_) {
    for (auto& button : controls_) {
      button->Draw(renderer);
    }
    menu_effects_.DrawPointers(renderer, pointer_buttons_);
  } else {
    for (auto& button : controls_) {
      button->Draw(renderer);
    }
  }
}

void KeyBindingsScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto render_size = context_.RenderSize();
  canvas_.SetViewportSize(
      {static_cast<float>(render_size.x), static_cast<float>(render_size.y)});
  canvas_.Layout(renderer);
}

void KeyBindingsScene::DrawWarning(engine::render::Renderer2D& renderer) {
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

void KeyBindingsScene::DrawRows(engine::render::Renderer2D& renderer) {
  if (rows_.empty()) {
    return;
  }
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const auto white = engine::render::Color::White();
  const auto binding_color = engine::render::Color::FromBytes(255, 255, 0, 255);
  const float font_size = constants::ui::Settings::kVolumeLabelFontSize;

  const auto& bindings = context_.KeyBindingSet();

  for (const auto& row : rows_) {
    if (!row.button) {
      continue;
    }
    const auto pos = row.button->GetPosition();
    const auto size = row.button->GetSize();
    if (size.x <= 0.0f || size.y <= 0.0f) {
      continue;
    }

    std::string value_text;
    if (is_binding_ && row.action == binding_action_) {
      value_text = "Press Key...";
    } else {
      auto key = bindings.Primary(row.action);
      value_text = KeyDisplayName(key);
    }

    const auto label_size = renderer.MeasureText(row.label, font_size);
    const auto value_size = renderer.MeasureText(value_text, font_size);
    const float label_x = pos.x + kRowTextPadding;
    const float label_y = pos.y + (size.y - label_size.y) * 0.5f;
    const float value_x = pos.x + size.x - kRowTextPadding - value_size.x;
    const float value_y = pos.y + (size.y - value_size.y) * 0.5f;

    auto color =
        (is_binding_ && row.action == binding_action_) ? binding_color : white;

    renderer.DrawText(row.label, {label_x, label_y}, font_size, white);
    renderer.DrawText(value_text, {value_x, value_y}, font_size, color);
  }
}

void KeyBindingsScene::HandleRebind(GameAction action) {
  if (is_binding_) return;
  is_binding_ = true;
  binding_action_ = action;
}

void KeyBindingsScene::RefreshButtons() {}

}  // namespace client
