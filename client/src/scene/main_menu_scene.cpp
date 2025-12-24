#include "main_menu_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "application.h"
#include "audio_paths.h"
#include "engine/core/engine_runtime.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {
constexpr float kButtonHeight = 72.0f;
constexpr float kButtonWidth = 170.0f;
constexpr const char* kTitleFont = "title_font";
constexpr const char* kBodyFont = "body_font";
constexpr float kPointerFrameDuration = 0.06f;
constexpr float kPointerSpacing = 28.0f;
constexpr float kPointerScaleFactor = 0.6f;
constexpr float kTitleScaleFactor = 1.3f;
constexpr char kTitleTexturePath[] = "assets/ui/main_menu_title.png";

void PlayUiSound(Application& app, const std::string& path) {
  if (path.empty()) {
    return;
  }
  if (auto audio = app.GetEngine().Audio()) {
    audio->PlaySoundEffect(path);
  }
}
}  // namespace

MainMenuScene::MainMenuScene(Application& app) : app_(app) {
  auto& renderer = app_.GetEngine().Renderer();

  renderer.LoadFont(kTitleFont, "assets/fonts/trajanpro_bold.otf");
  renderer.LoadFont(kBodyFont, "assets/fonts/Perpetua-Regular.otf");
  renderer.SetFont(kBodyFont);

  hover_sfx_path_ =
      ResolveAssetPath("assets/song/effects/change_selection.mp3");
  click_sfx_path_ = ResolveAssetPath("assets/song/effects/button_confirm.mp3");

  for (int i = 0; i <= 10; ++i) {
    std::ostringstream path;
    path << "assets/ui/main_menu_pointer_anim" << std::setw(4)
         << std::setfill('0') << i << ".png";
    auto tex = renderer.LoadTextureFromFile(path.str());
    if (tex) {
      pointer_frames_.push_back(tex);
    }
  }
  title_texture_ = renderer.LoadTextureFromFile(kTitleTexturePath);

  const auto white = engine::render::Color::White();

  play_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Start Game",
      [this]() {
        PlayUiSound(app_, click_sfx_path_);
        app_.OnPlay();
      });
  settings_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Options", [this]() {
        PlayUiSound(app_, click_sfx_path_);
        app_.OnOpenSettings();
      });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{kButtonWidth, kButtonHeight}, "Quit Game",
      [this]() {
        PlayUiSound(app_, click_sfx_path_);
        app_.OnQuitApplication();
      });

  ui_elements_.push_back(play_button_);
  ui_elements_.push_back(settings_button_);
  ui_elements_.push_back(quit_button_);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(48.0f));
  root->SetSpacing(36.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_slot = std::make_shared<engine::ui::BoxElement>();
  title_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  title_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(300.0f);
  title_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  title_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    title_rect_ = {rect.top_left_x_, rect.top_left_y_ - 12.0f, rect.width_,
                   rect.height_};
  });
  root->AddChild(title_slot);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(22.0f);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
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

  add_slot(play_button_);
  add_slot(settings_button_);
  add_slot(quit_button_);

  root->AddChild(button_column);

  canvas_.SetRoot(root);
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);
  play_button_->SetColors(transparent, transparent, transparent);
  settings_button_->SetColors(transparent, transparent, transparent);
  quit_button_->SetColors(transparent, transparent, transparent);
  play_button_->SetTextColor(white);
  settings_button_->SetTextColor(white);
  quit_button_->SetTextColor(white);
  play_button_->SetTextScale(0.46f);
  settings_button_->SetTextScale(0.46f);
  quit_button_->SetTextScale(0.46f);
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = app_.GetEngine().Renderer();
  LayoutUi(renderer);
  auto& input = app_.GetEngine().Input();
  const auto mouse_pos = input.GetMousePosition();

  std::array<std::shared_ptr<ui::Button>, 3> buttons{
      play_button_, settings_button_, quit_button_};
  const float max_elapsed =
      pointer_frames_.empty()
          ? 0.0f
          : (static_cast<float>(pointer_frames_.size() - 1) *
             kPointerFrameDuration);
  for (std::size_t i = 0; i < buttons.size(); ++i) {
    auto& button = buttons[i];
    if (!button) continue;
    auto& state = pointer_states_[i];
    state.was_hovered = state.hovered;
    state.hovered =
        engine::math::RectF{button->GetPosition(), button->GetSize()}.Contains(
            mouse_pos);
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

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
}

void MainMenuScene::Draw(engine::render::Renderer2D& renderer) {
  renderer.SetFont(kBodyFont);
  LayoutUi(renderer);

  canvas_.Draw(renderer);
  DrawTitle(renderer);
  renderer.SetFont(kTitleFont);
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  renderer.SetFont(kBodyFont);
  DrawPointers(renderer);
}

void MainMenuScene::LayoutUi(engine::render::Renderer2D& renderer) {
  renderer.SetFont(kBodyFont);
  const auto window_size = app_.GetEngine().Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
  canvas_.Layout(renderer);
}

void MainMenuScene::DrawPointers(engine::render::Renderer2D& renderer) {
  if (pointer_frames_.empty()) {
    return;
  }
  std::array<std::shared_ptr<ui::Button>, 3> buttons{
      play_button_, settings_button_, quit_button_};
  const std::size_t frame_count = pointer_frames_.size();
  for (std::size_t i = 0; i < buttons.size(); ++i) {
    auto& button = buttons[i];
    const auto& state = pointer_states_[i];
    if (!button || !state.hovered) continue;

    const float frame_pos = state.elapsed / kPointerFrameDuration;
    const std::size_t frame_index =
        std::min(frame_count - 1, static_cast<std::size_t>(frame_pos));
    auto texture = pointer_frames_[frame_index];
    if (!texture) continue;
    const auto tex_size = texture->GetSize();
    if (tex_size.y == 0) continue;

    const float scale = (kButtonHeight * 0.85f * kPointerScaleFactor) /
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
        static_cast<float>(tex_size.x), 0.0f, -static_cast<float>(tex_size.x),
        static_cast<float>(tex_size.y)};
    renderer.DrawTexture(*texture, right_params);
  }
}

void MainMenuScene::DrawTitle(engine::render::Renderer2D& renderer) {
  if (!title_texture_) {
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
  const float draw_scale = scale * kTitleScaleFactor;
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

}  // namespace client
