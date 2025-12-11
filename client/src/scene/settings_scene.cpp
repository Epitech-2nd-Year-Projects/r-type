#include "settings_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "ui/button.h"
#include "ui/label.h"

namespace client {

namespace {

std::string VolumeToString(float volume) {
  int percent = static_cast<int>(std::round(volume * 100.0f));
  return std::to_string(percent) + "%";
}

}  // namespace

SettingsScene::SettingsScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.SetFont("kenney_future");

  engine::render::Color white = engine::render::Color::White();
  float center_x = 1600.0f / 2.0f;
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
      }
      if (btn->GetText() == "+" || btn->GetText() == "-") {
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

void SettingsScene::Update(engine::time::TimeDelta dt) {
  auto& input = app_.GetEngine().Input();
  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }

  if (input.IsActionActive("Cancel")) {
    app_.OnQuitToMenu();
  }
}

void SettingsScene::Draw(engine::render::Renderer2D& renderer) {
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
}

}  // namespace client