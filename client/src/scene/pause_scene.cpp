#include "pause_scene.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "application.h"
#include "engine/core/engine_runtime.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace client {
namespace {

constexpr float kButtonWidth = 360.0f;
constexpr float kButtonHeight = 64.0f;
constexpr float kSmallButtonSize = 40.0f;

std::string VolumeToString(float value) {
  const int percent = static_cast<int>(std::round(value * 100.0f));
  return std::to_string(percent) + '%';
}

}  // namespace

PauseScene::PauseScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();
  renderer.LoadFont("times", "assets/fonts/times.ttf");
  renderer.SetFont("times");

  auto& input = app_.GetEngine().Input();
  pause_toggle_pressed_ =
      input.IsActionActive("Pause") || input.IsActionActive("Cancel");
  confirm_pressed_ = input.IsActionActive("Confirm");

  const auto white = engine::render::Color::White();
  const auto hover = engine::render::Color::FromBytes(220, 220, 220);
  const auto press = engine::render::Color::FromBytes(180, 180, 180);

  resume_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Resume",
      [this]() { app_.OnGameResume(); });
  options_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Hide Options",
      [this]() { ToggleOptions(); });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Quit to Main Menu",
      [this]() { app_.OnQuitToMenu(); });

  menu_buttons_.push_back(resume_button_);
  menu_buttons_.push_back(options_button_);
  menu_buttons_.push_back(quit_button_);

  const auto large_button_texture =
      renderer.LoadTextureFromFile("assets/ui/button_large.png");
  if (large_button_texture) {
    for (auto& button : menu_buttons_) {
      button->SetTexture(large_button_texture);
      button->SetColors(white, hover, press);
    }
  }

  auto change_music = [this](float delta) {
    if (auto audio = app_.GetEngine().Audio()) {
      const float next =
          std::clamp(audio->GetMusicVolume() + delta, 0.0f, 1.0f);
      audio->SetMusicVolume(next);
      RefreshVolumeLabels();
    }
  };

  auto change_sfx = [this](float delta) {
    if (auto audio = app_.GetEngine().Audio()) {
      const float next = std::clamp(audio->GetSfxVolume() + delta, 0.0f, 1.0f);
      audio->SetSfxVolume(next);
      RefreshVolumeLabels();
    }
  };

  music_minus_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kSmallButtonSize, kSmallButtonSize}, "-",
      [change_music]() { change_music(-0.05f); });
  music_plus_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kSmallButtonSize, kSmallButtonSize}, "+",
      [change_music]() { change_music(0.05f); });
  sfx_minus_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kSmallButtonSize, kSmallButtonSize}, "-",
      [change_sfx]() { change_sfx(-0.05f); });
  sfx_plus_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{kSmallButtonSize, kSmallButtonSize}, "+",
      [change_sfx]() { change_sfx(0.05f); });

  option_buttons_.push_back(music_minus_button_);
  option_buttons_.push_back(music_plus_button_);
  option_buttons_.push_back(sfx_minus_button_);
  option_buttons_.push_back(sfx_plus_button_);

  const auto small_button_texture =
      renderer.LoadTextureFromFile("assets/ui/button_small.png");
  if (small_button_texture) {
    for (auto& button : option_buttons_) {
      button->SetTexture(small_button_texture);
      button->SetColors(white, hover, press);
    }
  }

  title_ = std::make_shared<engine::ui::TextElement>(
      "Paused", engine::ui::FontSize::RelativeWidth(0.06f),
      engine::render::Color::White());
  title_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  hint_ = std::make_shared<engine::ui::TextElement>(
      "Pause: P (AZERTY) or Escape", engine::ui::FontSize::Pixels(20.0f),
      engine::render::Color::FromBytes(220, 220, 220));
  hint_->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  music_volume_label_ = std::make_shared<engine::ui::TextElement>(
      "0%", engine::ui::FontSize::Pixels(20.0f),
      engine::render::Color::White());
  sfx_volume_label_ = std::make_shared<engine::ui::TextElement>(
      "0%", engine::ui::FontSize::Pixels(20.0f),
      engine::render::Color::White());

  BuildUi();
  RefreshVolumeLabels();
}

void PauseScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();

  for (auto& button : menu_buttons_) {
    button->Update(dt, input);
  }
  if (options_open_) {
    for (auto& button : option_buttons_) {
      button->Update(dt, input);
    }
  }

  const bool pause_toggle =
      input.IsActionActive("Pause") || input.IsActionActive("Cancel");
  if (pause_toggle && !pause_toggle_pressed_) {
    app_.OnGameResume();
  }
  pause_toggle_pressed_ = pause_toggle;

  const bool confirm_pressed = input.IsActionActive("Confirm");
  if (confirm_pressed && !confirm_pressed_) {
    app_.OnGameResume();
  }
  confirm_pressed_ = confirm_pressed;
}

void PauseScene::Draw(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  renderer.DrawRect({0.0f, 0.0f, static_cast<float>(window_size.x),
                     static_cast<float>(window_size.y)},
                    engine::render::Color::FromBytes(6, 10, 22, 210));

  canvas_.Draw(renderer);
  for (auto& button : menu_buttons_) {
    button->Draw(renderer);
  }
  if (options_open_) {
    for (auto& button : option_buttons_) {
      button->Draw(renderer);
    }
  }
}

void PauseScene::BuildUi() {
  UpdateOptionsButtonLabel();
  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(28.0f));
  root->SetSpacing(14.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  root->AddChild(title_);
  root->AddChild(hint_);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(10.0f);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.width = engine::ui::LayoutValue::Pixels(kButtonWidth);
    slot->Layout().size.height =
        engine::ui::LayoutValue::Pixels(kButtonHeight + 8.0f);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      const float x = rect.top_left_x_;
      const float y = rect.top_left_y_ + (rect.height_ - kButtonHeight) * 0.5f;
      button->SetPosition({x, y});
      button->SetSize({rect.width_, kButtonHeight});
    });
    button_column->AddChild(slot);
  };

  add_slot(resume_button_);
  add_slot(options_button_);
  add_slot(quit_button_);

  root->AddChild(button_column);

  if (options_open_) {
    auto options_wrapper = std::make_shared<engine::ui::StackContainer>(
        engine::ui::Axis::kVertical);
    options_wrapper->SetSpacing(10.0f);
    options_wrapper->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    options_wrapper->Layout().margin.top = 20.0f;
    options_wrapper->SetChildAlignment(
        {engine::ui::HorizontalAlignment::kCenter,
         engine::ui::VerticalAlignment::kCenter});

    auto options_title = std::make_shared<engine::ui::TextElement>(
        "Options", engine::ui::FontSize::Pixels(26.0f),
        engine::render::Color::White());
    options_title->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    options_wrapper->AddChild(options_title);

    auto make_volume_row =
        [&](const std::string& label,
            const std::shared_ptr<ui::Button>& minus_button,
            const std::shared_ptr<engine::ui::TextElement>& value_label,
            const std::shared_ptr<ui::Button>& plus_button) {
          auto row = std::make_shared<engine::ui::StackContainer>(
              engine::ui::Axis::kHorizontal);
          row->SetSpacing(12.0f);
          row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                                  engine::ui::VerticalAlignment::kCenter});

          auto label_text = std::make_shared<engine::ui::TextElement>(
              label, engine::ui::FontSize::Pixels(22.0f),
              engine::render::Color::White());
          label_text->Layout().size.width =
              engine::ui::LayoutValue::Pixels(180.0f);
          row->AddChild(label_text);

          auto minus_slot = std::make_shared<engine::ui::BoxElement>();
          minus_slot->Layout().size = {
              engine::ui::LayoutValue::Pixels(kSmallButtonSize),
              engine::ui::LayoutValue::Pixels(kSmallButtonSize)};
          minus_slot->SetLayoutCallback(
              [minus_button](const engine::math::RectF& rect) {
                minus_button->SetPosition({rect.top_left_x_, rect.top_left_y_});
                minus_button->SetSize({rect.width_, rect.height_});
              });

          value_label->Layout().size.width =
              engine::ui::LayoutValue::Pixels(80.0f);
          value_label->Layout().alignment.horizontal =
              engine::ui::HorizontalAlignment::kCenter;

          auto plus_slot = std::make_shared<engine::ui::BoxElement>();
          plus_slot->Layout().size = {
              engine::ui::LayoutValue::Pixels(kSmallButtonSize),
              engine::ui::LayoutValue::Pixels(kSmallButtonSize)};
          plus_slot->SetLayoutCallback(
              [plus_button](const engine::math::RectF& rect) {
                plus_button->SetPosition({rect.top_left_x_, rect.top_left_y_});
                plus_button->SetSize({rect.width_, rect.height_});
              });

          row->AddChild(minus_slot);
          row->AddChild(value_label);
          row->AddChild(plus_slot);

          return row;
        };

    options_wrapper->AddChild(
        make_volume_row("Music Volume", music_minus_button_,
                        music_volume_label_, music_plus_button_));
    options_wrapper->AddChild(make_volume_row(
        "SFX Volume", sfx_minus_button_, sfx_volume_label_, sfx_plus_button_));

    root->AddChild(options_wrapper);
  }

  canvas_.SetRoot(root);
}

void PauseScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

void PauseScene::RefreshVolumeLabels() {
  if (!music_volume_label_ || !sfx_volume_label_) {
    return;
  }

  if (auto audio = app_.GetEngine().Audio()) {
    const float music_volume = audio->GetMusicVolume();
    const float sfx_volume = audio->GetSfxVolume();
    const bool music_changed =
        std::abs(music_volume - last_music_volume_) > 0.0001f;
    const bool sfx_changed = std::abs(sfx_volume - last_sfx_volume_) > 0.0001f;
    if (!music_changed && !sfx_changed) {
      return;
    }
    if (music_changed) {
      music_volume_label_->SetText(VolumeToString(music_volume));
      last_music_volume_ = music_volume;
    }
    if (sfx_changed) {
      sfx_volume_label_->SetText(VolumeToString(sfx_volume));
      last_sfx_volume_ = sfx_volume;
    }
    return;
  }

  music_volume_label_->SetText("N/A");
  sfx_volume_label_->SetText("N/A");
  last_music_volume_ = -1.0f;
  last_sfx_volume_ = -1.0f;
}

void PauseScene::ToggleOptions() {
  options_open_ = !options_open_;
  BuildUi();
  LayoutUi(app_.GetEngine().Renderer());
}

void PauseScene::UpdateOptionsButtonLabel() {
  if (!options_button_) {
    return;
  }
  options_button_->SetText(options_open_ ? "Hide Options" : "Show Options");
}

}  // namespace client
