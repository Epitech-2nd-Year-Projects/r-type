#include "settings_scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "engine/ui/types.h"
#include "key_bindings.h"
#include "ui/button.h"

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

SettingsScene::SettingsScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  engine::render::Color white = engine::render::Color::White();

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(48.0f));
  root->SetSpacing(20.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_text = std::make_shared<engine::ui::TextElement>(
      "Settings", engine::ui::FontSize::RelativeWidth(0.1f), white);
  title_text->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  root->AddChild(title_text);

  auto content =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  content->SetSpacing(15.0f);
  content->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto audio = app_.GetEngine().Audio();
  float current_music_vol = audio ? audio->GetMusicVolume() : 0.0f;
  float current_sfx_vol = audio ? audio->GetSfxVolume() : 0.0f;

  auto btn_tex = renderer.LoadTextureFromFile("assets/ui/button_large.png");
  auto small_btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_small.png");
  const auto hover = engine::render::Color::FromBytes(220, 220, 220);
  const auto press = engine::render::Color::FromBytes(180, 180, 180);

  auto make_volume_slider = [&](const std::string& label, float initial_volume,
                                auto on_change) {
    auto row = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kHorizontal);
    row->SetSpacing(15.0f);
    row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                            engine::ui::VerticalAlignment::kCenter});

    auto label_elem = std::make_shared<engine::ui::TextElement>(
        label, engine::ui::FontSize::Pixels(24.0f), white);
    label_elem->Layout().size.width = engine::ui::LayoutValue::Pixels(200.0f);
    row->AddChild(label_elem);

    auto volume_label = std::make_shared<engine::ui::TextElement>(
        VolumeToString(initial_volume), engine::ui::FontSize::Pixels(24.0f),
        white);
    volume_label->Layout().size.width = engine::ui::LayoutValue::Pixels(80.0f);
    volume_label->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;

    auto add_button_slot = [&](const std::shared_ptr<ui::Button>& button) {
      buttons_.push_back(button);
      auto slot = std::make_shared<engine::ui::BoxElement>();
      slot->Layout().size = {engine::ui::LayoutValue::Pixels(40.0f),
                             engine::ui::LayoutValue::Pixels(40.0f)};
      slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
        button->SetPosition({rect.top_left_x_, rect.top_left_y_});
        button->SetSize({rect.width_, rect.height_});
      });
      row->AddChild(slot);
    };

    auto minus_btn = std::make_shared<ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{40.0f, 40.0f}, "-",
        [this, on_change, volume_label]() {
          if (auto audio = app_.GetEngine().Audio()) {
            float v = on_change(audio, -0.1f);
            volume_label->SetText(VolumeToString(v));
          }
        });
    minus_btn->SetTexture(small_btn_tex);
    minus_btn->SetColors(white, hover, press);
    add_button_slot(minus_btn);

    row->AddChild(volume_label);

    auto plus_btn = std::make_shared<ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{40.0f, 40.0f}, "+",
        [this, on_change, volume_label]() {
          if (auto audio = app_.GetEngine().Audio()) {
            float v = on_change(audio, 0.1f);
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
      "Controls", engine::ui::FontSize::Pixels(28.0f), white);
  controls_title->Layout().margin.top = 20.0f;
  content->AddChild(controls_title);

  rebind_status_label_ = std::make_shared<engine::ui::TextElement>(
      "Click a binding to remap", engine::ui::FontSize::Pixels(18.0f),
      engine::render::Color::FromBytes(200, 200, 200));
  content->AddChild(rebind_status_label_);

  const auto keys = BindableKeys();
  key_state_buffer_.assign(keys.size(), false);

  auto begin_rebind = [this](GameAction action) {
    pending_rebind_ = action;
    if (rebind_status_label_) {
      rebind_status_label_->SetText("Press a key for " + ActionLabel(action));
    }
    if (auto row = FindRow(action)) {
      if (row->get().button) {
        row->get().button->SetText("Press key...");
      }
    }
    auto& input = app_.GetEngine().Input();
    RefreshKeyStateBuffer(input, key_state_buffer_);
  };

  auto bindings_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  bindings_column->SetSpacing(10.0f);
  bindings_column->Layout().margin.top = 10.0f;

  for (const GameAction action : kRebindableActions) {
    auto row = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kHorizontal);
    row->SetSpacing(15.0f);
    row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                            engine::ui::VerticalAlignment::kCenter});

    auto action_label = std::make_shared<engine::ui::TextElement>(
        ActionLabel(action), engine::ui::FontSize::Pixels(22.0f), white);
    action_label->Layout().size.width = engine::ui::LayoutValue::Pixels(150.0f);
    row->AddChild(action_label);

    auto button = std::make_shared<ui::Button>(
        engine::math::Vector2f{}, engine::math::Vector2f{280.0f, 45.0f},
        KeyDisplayName(app_.key_bindings().Primary(action)),
        [begin_rebind, action]() { begin_rebind(action); });
    button->SetTexture(btn_tex);
    button->SetColors(white, hover, press);
    buttons_.push_back(button);
    binding_rows_.emplace_back(action, button);

    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().size = {engine::ui::LayoutValue::Pixels(280.0f),
                           engine::ui::LayoutValue::Pixels(45.0f)};
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition({rect.top_left_x_, rect.top_left_y_});
      button->SetSize({rect.width_, rect.height_});
    });
    row->AddChild(slot);
    bindings_column->AddChild(row);
  }
  content->AddChild(bindings_column);

  auto fullscreen_btn = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{400.0f, 50.0f},
      "Toggle Fullscreen",
      [this]() { app_.GetEngine().Window().ToggleFullscreen(); });
  fullscreen_btn->SetTexture(btn_tex);
  fullscreen_btn->SetColors(white, hover, press);
  buttons_.push_back(fullscreen_btn);
  auto fullscreen_slot = std::make_shared<engine::ui::BoxElement>();
  fullscreen_slot->Layout().size = {engine::ui::LayoutValue::Pixels(400.0f),
                                    engine::ui::LayoutValue::Pixels(50.0f)};
  fullscreen_slot->Layout().margin.top = 20.0f;
  fullscreen_slot->SetLayoutCallback(
      [fullscreen_btn](const engine::math::RectF& rect) {
        fullscreen_btn->SetPosition({rect.top_left_x_, rect.top_left_y_});
        fullscreen_btn->SetSize({rect.width_, rect.height_});
      });
  content->AddChild(fullscreen_slot);

  root->AddChild(content);

  auto back_btn = std::make_shared<ui::Button>(
      engine::math::Vector2f{}, engine::math::Vector2f{400.0f, 50.0f}, "Back",
      [this]() { app_.OnQuitToMenu(); });
  back_btn->SetTexture(btn_tex);
  back_btn->SetColors(white, hover, press);
  buttons_.push_back(back_btn);
  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().size = {engine::ui::LayoutValue::Pixels(400.0f),
                              engine::ui::LayoutValue::Pixels(50.0f)};
  back_slot->Layout().alignment.vertical =
      engine::ui::VerticalAlignment::kStretch;
  back_slot->Layout().margin.top = 20.0f;
  back_slot->SetLayoutCallback([back_btn](const engine::math::RectF& rect) {
    back_btn->SetPosition({rect.top_left_x_, rect.top_left_y_});
    back_btn->SetSize({rect.width_, rect.height_});
  });

  // This will push the back button to the bottom if root is a V-Stack with
  // space-between or similar. With kCenter it's tricky. Let's add a spacer.
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
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();
  for (auto& btn : buttons_) {
    btn->Update(dt, input);
  }

  if (pending_rebind_) {
    if (input.IsKeyDown(engine::input::Key::kEscape)) {
      if (rebind_status_label_)
        rebind_status_label_->SetText("Rebind canceled");
      if (auto row = FindRow(*pending_rebind_)) {
        if (row->get().button) {
          row->get().button->SetText(
              KeyDisplayName(app_.key_bindings().Primary(row->get().action)));
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
        bool conflict = false;
        for (GameAction other : app_.key_bindings().Actions()) {
          if (other == action) continue;
          if (app_.key_bindings().Primary(other) == keys[i]) {
            conflict = true;
            if (rebind_status_label_) {
              rebind_status_label_->SetText("Key already bound to " +
                                            ActionLabel(other));
            }
            break;
          }
        }
        if (conflict) {
          RefreshKeyStateBuffer(input, key_state_buffer_);
          break;
        }

        const bool saved = app_.UpdateKeyBinding(action, keys[i]);
        if (auto row = FindRow(action)) {
          if (row->get().button) {
            row->get().button->SetText(
                KeyDisplayName(app_.key_bindings().Primary(action)));
          }
        }
        if (rebind_status_label_) {
          const std::string status = saved
                                         ? "Bound " + ActionLabel(action) +
                                               " to " + KeyDisplayName(keys[i])
                                         : "Failed to save key bindings";
          rebind_status_label_->SetText(status);
        }
        pending_rebind_.reset();
        RefreshKeyStateBuffer(input, key_state_buffer_);
        break;
      }
      key_state_buffer_[i] = down;
    }
  } else {
    RefreshKeyStateBuffer(input, key_state_buffer_);
    if (input.IsActionActive("Cancel")) {
      app_.OnQuitToMenu();
    }
  }
}

void SettingsScene::Draw(engine::render::Renderer2D& renderer) {
  LayoutUi(renderer);
  canvas_.Draw(renderer);
  for (auto& btn : buttons_) {
    btn->Draw(renderer);
  }
}

void SettingsScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

}  // namespace client
