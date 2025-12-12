#include "settings_scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "key_bindings.h"
#include "ui/button.h"
#include "ui/label.h"

namespace client {

namespace {

constexpr std::array<GameAction, 5> kRebindableActions{
    GameAction::kMoveUp, GameAction::kMoveDown, GameAction::kMoveLeft,
    GameAction::kMoveRight, GameAction::kShoot};

constexpr float kBindingRowSpacing = 60.0f;

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
  renderer.LoadFont("kenney_future", "assets/ui/kenney_future.ttf");
  renderer.SetFont("kenney_future");

  engine::render::Color white = engine::render::Color::White();
  float center_x =
      static_cast<float>(app_.GetEngine().Window().GetSize().x) * 0.5f;
  float start_y = 200.0f;

  ui_elements_.push_back(std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 140.0f, 100.0f}, "Settings", 60.0f,
      white));

  auto audio = app_.GetEngine().Audio();
  float current_music_vol = audio ? audio->GetMusicVolume() : 0.0f;
  float current_sfx_vol = audio ? audio->GetSfxVolume() : 0.0f;

  ui_elements_.push_back(std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 350.0f, start_y + 10.0f},
      "Music Volume", 24.0f, white));

  ui_elements_.push_back(std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x - 80.0f, start_y},
      engine::math::Vector2f{40.0f, 40.0f}, "-", [this]() {
        if (auto audio = app_.GetEngine().Audio()) {
          float v = audio->GetMusicVolume();
          v = std::max(0.0f, v - 0.1f);
          audio->SetMusicVolume(v);
          if (music_volume_label_)
            music_volume_label_->SetText(VolumeToString(v));
        }
      }));

  music_volume_label_ = std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 10.0f, start_y + 10.0f},
      VolumeToString(current_music_vol), 24.0f, white);
  ui_elements_.push_back(music_volume_label_);

  ui_elements_.push_back(std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x + 60.0f, start_y},
      engine::math::Vector2f{40.0f, 40.0f}, "+", [this]() {
        if (auto audio = app_.GetEngine().Audio()) {
          float v = audio->GetMusicVolume();
          v = std::min(1.0f, v + 0.1f);
          audio->SetMusicVolume(v);
          if (music_volume_label_)
            music_volume_label_->SetText(VolumeToString(v));
        }
      }));

  start_y += 100.0f;

  ui_elements_.push_back(std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 350.0f, start_y + 10.0f}, "SFX Volume",
      24.0f, white));

  ui_elements_.push_back(std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x - 80.0f, start_y},
      engine::math::Vector2f{40.0f, 40.0f}, "-", [this]() {
        if (auto audio = app_.GetEngine().Audio()) {
          float v = audio->GetSfxVolume();
          v = std::max(0.0f, v - 0.1f);
          audio->SetSfxVolume(v);
          if (sfx_volume_label_) sfx_volume_label_->SetText(VolumeToString(v));
        }
      }));

  sfx_volume_label_ = std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 10.0f, start_y + 10.0f},
      VolumeToString(current_sfx_vol), 24.0f, white);
  ui_elements_.push_back(sfx_volume_label_);

  ui_elements_.push_back(std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x + 60.0f, start_y},
      engine::math::Vector2f{40.0f, 40.0f}, "+", [this]() {
        if (auto audio = app_.GetEngine().Audio()) {
          float v = audio->GetSfxVolume();
          v = std::min(1.0f, v + 0.1f);
          audio->SetSfxVolume(v);
          if (sfx_volume_label_) sfx_volume_label_->SetText(VolumeToString(v));
        }
      }));

  start_y += 120.0f;

  ui_elements_.push_back(std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 350.0f, start_y + 10.0f}, "Controls",
      28.0f, white));

  rebind_status_label_ = std::make_shared<ui::Label>(
      engine::math::Vector2f{center_x - 350.0f, start_y + 50.0f},
      "Click a binding to remap", 18.0f,
      engine::render::Color::FromBytes(200, 200, 200));
  ui_elements_.push_back(rebind_status_label_);

  const auto keys = BindableKeys();
  key_state_buffer_.assign(keys.size(), false);

  const float bindings_y = start_y + 90.0f;
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

  for (std::size_t i = 0; i < kRebindableActions.size(); ++i) {
    const GameAction action = kRebindableActions[i];
    const float row_y = bindings_y + static_cast<float>(i) * kBindingRowSpacing;

    auto label = std::make_shared<ui::Label>(
        engine::math::Vector2f{center_x - 350.0f, row_y + 10.0f},
        ActionLabel(action), 22.0f, white);

    auto button = std::make_shared<ui::Button>(
        engine::math::Vector2f{center_x - 40.0f, row_y},
        engine::math::Vector2f{280.0f, 45.0f},
        KeyDisplayName(app_.key_bindings().Primary(action)),
        [begin_rebind, action]() { begin_rebind(action); });

    binding_rows_.emplace_back(action, label, button);
    ui_elements_.push_back(label);
    ui_elements_.push_back(button);
  }

  start_y = bindings_y +
            static_cast<float>(kRebindableActions.size()) * kBindingRowSpacing +
            60.0f;

  ui_elements_.push_back(std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x - 200.0f, start_y},
      engine::math::Vector2f{400.0f, 50.0f}, "Toggle Fullscreen",
      [this]() { app_.GetEngine().Window().ToggleFullscreen(); }));

  auto back_btn = std::make_shared<ui::Button>(
      engine::math::Vector2f{center_x - 200.0f, 750.0f},
      engine::math::Vector2f{400.0f, 50.0f}, "Back",
      [this]() { app_.OnQuitToMenu(); });
  ui_elements_.push_back(back_btn);

  auto btn_tex = renderer.LoadTextureFromFile("assets/ui/button_large.png");
  auto small_btn_tex =
      renderer.LoadTextureFromFile("assets/ui/button_small.png");

  if (btn_tex) {
    back_btn->SetTexture(btn_tex);
    back_btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220),
                        engine::render::Color::FromBytes(180, 180, 180));
  }

  for (auto& elem : ui_elements_) {
    if (auto btn = std::dynamic_pointer_cast<ui::Button>(elem)) {
      if (btn == back_btn) continue;

      if (btn->GetText() == "Toggle Fullscreen") {
        if (btn_tex) btn->SetTexture(btn_tex);
        btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220),
                       engine::render::Color::FromBytes(180, 180, 180));
      } else if (btn->GetText() == "+" || btn->GetText() == "-") {
        if (small_btn_tex)
          btn->SetTexture(small_btn_tex);
        else if (btn_tex)
          btn->SetTexture(btn_tex);
        btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220),
                       engine::render::Color::FromBytes(180, 180, 180));
      } else {
        if (small_btn_tex)
          btn->SetTexture(small_btn_tex);
        else if (btn_tex)
          btn->SetTexture(btn_tex);
        btn->SetColors(white, engine::render::Color::FromBytes(220, 220, 220),
                       engine::render::Color::FromBytes(180, 180, 180));
      }
    }
  }
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
  auto& input = app_.GetEngine().Input();
  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
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
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
}

}  // namespace client
