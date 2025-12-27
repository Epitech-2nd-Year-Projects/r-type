#include "settings_scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/audio/audio_engine.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "engine/ui/button.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "engine/ui/types.h"
#include "input/key_bindings.h"

namespace client {

namespace {

constexpr std::array<GameAction, 5> kRebindableActions{
    GameAction::kMoveUp, GameAction::kMoveDown, GameAction::kMoveLeft,
    GameAction::kMoveRight, GameAction::kShoot};

std::string VolumeToString(float volume) {
  int percent = static_cast<int>(std::round(volume * 100.0f));
  return std::to_string(percent) + "%";
}

void RefreshKeyStateBuffer(engine::input::InputManager& input,
                           std::vector<bool>& buffer) {
  const auto keys = BindableKeys();
  if (buffer.size() != keys.size()) buffer.assign(keys.size(), false);
  for (std::size_t i = 0; i < keys.size(); ++i) {
    buffer[i] = input.IsKeyDown(keys[i]);
  }
}

}  // namespace

SettingsScene::SettingsScene(ClientContext& context) : context_(context) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  assets.LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  const auto white = constants::ui::kButtonBaseColor;

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(
      engine::ui::Insets::Uniform(constants::ui::Settings::kRootPadding));
  root->SetSpacing(constants::ui::Settings::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_text = std::make_shared<engine::ui::TextElement>(
      "Settings",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Settings::kTitleFontScale),
      white);
  title_text->SetFont(std::string(constants::ui::kTitleFont));
  title_text->SetFontFallback(std::string(constants::ui::kBodyFont));
  title_text->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text);

  auto content =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  content->SetSpacing(constants::ui::Settings::kContentSpacing);
  content->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto audio = context_.Audio();
  float current_music_vol = audio ? audio->GetMusicVolume() : 0.0f;
  float current_sfx_vol = audio ? audio->GetSfxVolume() : 0.0f;

  auto btn_tex = assets.GetTexture(constants::ui::kButtonTextureLargePath);
  auto small_btn_tex = assets.GetTexture(constants::ui::kButtonTextureSmallPath);
  const auto hover = constants::ui::kButtonHoverColor;
  const auto press = constants::ui::kButtonPressColor;

  auto make_volume_slider = [&](const std::string& label, float initial_volume,
                                auto on_change) {
    auto row = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kHorizontal);
    row->SetSpacing(constants::ui::Settings::kVolumeRowSpacing);
    row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                            engine::ui::VerticalAlignment::kCenter});

    auto label_elem = std::make_shared<engine::ui::TextElement>(
        label,
        engine::ui::FontSize::Pixels(
            constants::ui::Settings::kVolumeLabelFontSize),
        white);
    label_elem->Layout().size.width = engine::ui::LayoutValue::Pixels(
        constants::ui::Settings::kVolumeLabelWidth);
    row->AddChild(label_elem);

    auto volume_label = std::make_shared<engine::ui::TextElement>(
        VolumeToString(initial_volume),
        engine::ui::FontSize::Pixels(
            constants::ui::Settings::kVolumeLabelFontSize),
        white);
    volume_label->Layout().size.width = engine::ui::LayoutValue::Pixels(
        constants::ui::Settings::kVolumeValueWidth);
    volume_label->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;

    auto add_button_slot =
        [&](const std::shared_ptr<engine::ui::Button>& button) {
          buttons_.push_back(button);
          auto slot = std::make_shared<engine::ui::BoxElement>();
          slot->Layout().size = {
              engine::ui::LayoutValue::Pixels(
                  constants::ui::Settings::kVolumeButtonSize),
              engine::ui::LayoutValue::Pixels(
                  constants::ui::Settings::kVolumeButtonSize)};
          slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
            button->SetPosition({rect.top_left_x_, rect.top_left_y_});
            button->SetSize({rect.width_, rect.height_});
          });
          row->AddChild(slot);
        };

    auto minus_btn = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{constants::ui::Settings::kVolumeButtonSize,
                               constants::ui::Settings::kVolumeButtonSize},
        "-", [this, on_change, volume_label]() {
          if (auto audio = context_.Audio()) {
            float v = on_change(audio, -constants::ui::Settings::kVolumeStep);
            volume_label->SetText(VolumeToString(v));
          }
        });
    minus_btn->SetTexture(small_btn_tex);
    minus_btn->SetColors(white, hover, press);
    add_button_slot(minus_btn);

    row->AddChild(volume_label);

    auto plus_btn = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{constants::ui::Settings::kVolumeButtonSize,
                               constants::ui::Settings::kVolumeButtonSize},
        "+", [this, on_change, volume_label]() {
          if (auto audio = context_.Audio()) {
            float v = on_change(audio, constants::ui::Settings::kVolumeStep);
            volume_label->SetText(VolumeToString(v));
          }
        });
    plus_btn->SetTexture(small_btn_tex);
    plus_btn->SetColors(white, hover, press);
    add_button_slot(plus_btn);

    content->AddChild(row);
    return volume_label;
  };

  music_volume_label_ = make_volume_slider(
      "Music Volume", current_music_vol, [](auto audio, float delta) {
        float v = audio->GetMusicVolume();
        v = std::clamp(v + delta, 0.0f, 1.0f);
        audio->SetMusicVolume(v);
        return v;
      });

  sfx_volume_label_ = make_volume_slider(
      "SFX Volume", current_sfx_vol, [](auto audio, float delta) {
        float v = audio->GetSfxVolume();
        v = std::clamp(v + delta, 0.0f, 1.0f);
        audio->SetSfxVolume(v);
        return v;
      });

  auto controls_title = std::make_shared<engine::ui::TextElement>(
      "Controls",
      engine::ui::FontSize::Pixels(
          constants::ui::Settings::kControlsTitleFontSize),
      white);
  controls_title->SetFont(std::string(constants::ui::kTitleFont));
  controls_title->SetFontFallback(std::string(constants::ui::kBodyFont));
  controls_title->Layout().margin.top =
      constants::ui::Settings::kControlsTitleMarginTop;
  content->AddChild(controls_title);

  rebind_status_label_ = std::make_shared<engine::ui::TextElement>(
      context_.KeyBindingServiceRef().IdleMessage(),
      engine::ui::FontSize::Pixels(
          constants::ui::Settings::kRebindStatusFontSize),
      constants::ui::Settings::kRebindStatusColor);
  content->AddChild(rebind_status_label_);

  const auto keys = BindableKeys();
  key_state_buffer_.assign(keys.size(), false);

  auto begin_rebind = [this](GameAction action) {
    pending_rebind_ = action;
    if (rebind_status_label_) {
      rebind_status_label_->SetText(
          context_.KeyBindingServiceRef().PromptMessage(action));
    }
    if (auto row = FindRow(action)) {
      if (row->get().button) {
        row->get().button->SetText("Press key...");
      }
    }
    auto& input = context_.Input();
    RefreshKeyStateBuffer(input, key_state_buffer_);
  };

  auto bindings_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  bindings_column->SetSpacing(constants::ui::Settings::kBindingColumnSpacing);
  bindings_column->Layout().margin.top =
      constants::ui::Settings::kBindingColumnMarginTop;

  for (const GameAction action : kRebindableActions) {
    auto row = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kHorizontal);
    row->SetSpacing(constants::ui::Settings::kBindingRowSpacing);
    row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                            engine::ui::VerticalAlignment::kCenter});

    auto action_label = std::make_shared<engine::ui::TextElement>(
        ActionLabel(action),
        engine::ui::FontSize::Pixels(
            constants::ui::Settings::kBindingLabelFontSize),
        white);
    action_label->Layout().size.width = engine::ui::LayoutValue::Pixels(
        constants::ui::Settings::kBindingLabelWidth);
    row->AddChild(action_label);

    auto button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{constants::ui::Settings::kBindingButtonWidth,
                               constants::ui::Settings::kBindingButtonHeight},
        KeyDisplayName(context_.KeyBindingSet().Primary(action)),
        [begin_rebind, action]() { begin_rebind(action); });
    button->SetTexture(btn_tex);
    button->SetColors(white, hover, press);
    buttons_.push_back(button);
    binding_rows_.emplace_back(action, button);

    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().size = {engine::ui::LayoutValue::Pixels(
                               constants::ui::Settings::kBindingButtonWidth),
                           engine::ui::LayoutValue::Pixels(
                               constants::ui::Settings::kBindingButtonHeight)};
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition({rect.top_left_x_, rect.top_left_y_});
      button->SetSize({rect.width_, rect.height_});
    });
    row->AddChild(slot);
    bindings_column->AddChild(row);
  }
  content->AddChild(bindings_column);

  auto fullscreen_btn = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Settings::kFullscreenButtonWidth,
                             constants::ui::Settings::kFullscreenButtonHeight},
      "Toggle Fullscreen", [this]() { context_.Window().ToggleFullscreen(); });
  fullscreen_btn->SetTexture(btn_tex);
  fullscreen_btn->SetColors(white, hover, press);
  buttons_.push_back(fullscreen_btn);
  auto fullscreen_slot = std::make_shared<engine::ui::BoxElement>();
  fullscreen_slot->Layout().size = {
      engine::ui::LayoutValue::Pixels(
          constants::ui::Settings::kFullscreenButtonWidth),
      engine::ui::LayoutValue::Pixels(
          constants::ui::Settings::kFullscreenButtonHeight)};
  fullscreen_slot->Layout().margin.top =
      constants::ui::Settings::kFullscreenMarginTop;
  fullscreen_slot->SetLayoutCallback(
      [fullscreen_btn](const engine::math::RectF& rect) {
        fullscreen_btn->SetPosition({rect.top_left_x_, rect.top_left_y_});
        fullscreen_btn->SetSize({rect.width_, rect.height_});
      });
  content->AddChild(fullscreen_slot);

  root->AddChild(content);

  auto back_btn = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Settings::kBackButtonWidth,
                             constants::ui::Settings::kBackButtonHeight},
      "Back", [this]() { context_.OnCloseSettings(); });
  back_btn->SetTexture(btn_tex);
  back_btn->SetColors(white, hover, press);
  buttons_.push_back(back_btn);
  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().size = {engine::ui::LayoutValue::Pixels(
                                  constants::ui::Settings::kBackButtonWidth),
                              engine::ui::LayoutValue::Pixels(
                                  constants::ui::Settings::kBackButtonHeight)};
  back_slot->Layout().alignment.vertical =
      engine::ui::VerticalAlignment::kStretch;
  back_slot->Layout().margin.top = constants::ui::Settings::kBackMarginTop;
  back_slot->SetLayoutCallback([back_btn](const engine::math::RectF& rect) {
    back_btn->SetPosition({rect.top_left_x_, rect.top_left_y_});
    back_btn->SetSize({rect.width_, rect.height_});
  });

  auto spacer = std::make_shared<engine::ui::BoxElement>();
  spacer->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->AddChild(spacer);

  root->AddChild(back_slot);

  canvas_.SetRoot(root);
}

std::optional<std::reference_wrapper<SettingsScene::BindingRow>>
SettingsScene::FindRow(GameAction action) {
  for (auto& row : binding_rows_) {
    if (row.action == action) {
      return row;
    }
  }
  return std::nullopt;
}

void SettingsScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();
  for (auto& btn : buttons_) {
    btn->Update(dt, input);
  }

  if (pending_rebind_) {
    if (input.IsKeyDown(engine::input::Key::kEscape)) {
      if (rebind_status_label_) {
        rebind_status_label_->SetText(
            context_.KeyBindingServiceRef().CancelMessage());
      }
      if (auto row = FindRow(*pending_rebind_)) {
        if (row->get().button) {
          row->get().button->SetText(KeyDisplayName(
              context_.KeyBindingSet().Primary(row->get().action)));
        }
      }
      pending_rebind_.reset();
      RefreshKeyStateBuffer(input, key_state_buffer_);
      return;
    }

    const auto keys = BindableKeys();
    if (key_state_buffer_.size() != keys.size()) {
      key_state_buffer_.assign(keys.size(), false);
    }

    for (std::size_t i = 0; i < keys.size(); ++i) {
      const bool down = input.IsKeyDown(keys[i]);
      const bool was_down = key_state_buffer_[i];
      if (down && !was_down) {
        const GameAction action = *pending_rebind_;
        const auto result = context_.UpdateKeyBinding(action, keys[i]);
        if (result.status == KeyBindingUpdateStatus::kConflict) {
          if (rebind_status_label_) {
            rebind_status_label_->SetText(result.message);
          }
          RefreshKeyStateBuffer(input, key_state_buffer_);
          break;
        }

        if (auto row = FindRow(action)) {
          if (row->get().button) {
            row->get().button->SetText(
                KeyDisplayName(context_.KeyBindingSet().Primary(action)));
          }
        }
        if (rebind_status_label_) {
          rebind_status_label_->SetText(result.message);
        }
        pending_rebind_.reset();
        RefreshKeyStateBuffer(input, key_state_buffer_);
        break;
      }
      key_state_buffer_[i] = down;
    }
  } else {
    RefreshKeyStateBuffer(input, key_state_buffer_);
    if (input.IsActionActive(std::string(constants::input::kActionCancel))) {
      context_.OnCloseSettings();
    }
  }
}

void SettingsScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  for (auto& btn : buttons_) {
    btn->Draw(renderer);
  }
}

void SettingsScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
