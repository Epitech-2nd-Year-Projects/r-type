#include "main_menu_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "audio_paths.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/audio/audio_engine.h"
#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/render/renderer2d.h"
#include "logging.h"

namespace client {

namespace {

void PlayUiSound(ClientContext& context, const std::string& path) {
  if (path.empty()) {
    return;
  }
  if (auto audio = context.Audio()) {
    audio->PlaySoundEffect(path);
  }
}
}  // namespace

MainMenuScene::MainMenuScene(ClientContext& context) : context_(context) {
  auto& renderer = context_.Renderer();

  renderer.LoadFont(std::string(constants::ui::kTitleFont),
                    std::string(constants::ui::kTitleFontPath));
  renderer.LoadFont(std::string(constants::ui::kBodyFont),
                    std::string(constants::ui::kBodyFontPath));
  renderer.SetFont(std::string(constants::ui::kBodyFont));

  hover_sfx_path_ = ResolveAssetPath(constants::ui::kMenuHoverSfxPath);
  click_sfx_path_ = ResolveAssetPath(constants::ui::kMenuClickSfxPath);

  for (int i = 0; i < constants::ui::MainMenu::kPointerFrameCount; ++i) {
    std::ostringstream path;
    path << constants::ui::kMenuPointerFramePrefix << std::setw(4)
         << std::setfill('0') << i << constants::ui::kMenuPointerFrameExtension;
    auto tex = renderer.LoadTextureFromFile(path.str());
    if (tex) {
      pointer_frames_.push_back(tex);
    }
  }
  title_texture_ = renderer.LoadTextureFromFile(
      std::string(constants::ui::MainMenu::kTitleTexturePath));

  const auto white = engine::render::Color::White();

  play_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Start Game", [this]() {
        PlayUiSound(context_, click_sfx_path_);
        context_.OnPlay();
      });
  settings_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Options", [this]() {
        PlayUiSound(context_, click_sfx_path_);
        context_.OnOpenSettings();
      });
  quit_button_ = std::make_shared<ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::MainMenu::kButtonWidth,
                             constants::ui::MainMenu::kButtonHeight},
      "Quit Game", [this]() {
        PlayUiSound(context_, click_sfx_path_);
        context_.OnQuitApplication();
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
  root->SetPadding(
      engine::ui::Insets::Uniform(constants::ui::MainMenu::kRootPadding));
  root->SetSpacing(constants::ui::MainMenu::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_slot = std::make_shared<engine::ui::BoxElement>();
  title_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  title_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::MainMenu::kTitleSlotHeight);
  title_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  title_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    title_rect_ = {rect.top_left_x_,
                   rect.top_left_y_ + constants::ui::MainMenu::kTitleYOffset,
                   rect.width_, rect.height_};
  });
  root->AddChild(title_slot);

  auto button_column =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  button_column->SetSpacing(constants::ui::MainMenu::kButtonColumnSpacing);
  button_column->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;

  auto add_slot = [&](const std::shared_ptr<ui::Button>& button) {
    auto slot = std::make_shared<engine::ui::BoxElement>();
    slot->Layout().alignment.horizontal =
        engine::ui::HorizontalAlignment::kCenter;
    slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
        constants::ui::MainMenu::kButtonHeight +
        constants::ui::MainMenu::kButtonSlotPadding);
    slot->SetLayoutCallback([button](const engine::math::RectF& rect) {
      button->SetPosition(
          {rect.top_left_x_ +
               (rect.width_ - constants::ui::MainMenu::kButtonWidth) * 0.5f,
           rect.top_left_y_ + constants::ui::MainMenu::kButtonSlotInset});
      button->SetSize({constants::ui::MainMenu::kButtonWidth,
                       constants::ui::MainMenu::kButtonHeight});
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
  play_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
  settings_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
  quit_button_->SetTextScale(constants::ui::MainMenu::kButtonTextScale);
}

void MainMenuScene::Update(engine::time::TimeDelta dt) {
  auto& renderer = context_.Renderer();
  LayoutUi(renderer);
  auto& input = context_.Input();
  const auto mouse_pos = input.GetMousePosition();

  std::array<std::shared_ptr<ui::Button>, 3> buttons{
      play_button_, settings_button_, quit_button_};
  const float max_elapsed =
      pointer_frames_.empty()
          ? 0.0f
          : (static_cast<float>(pointer_frames_.size() - 1) *
             constants::ui::MainMenu::kPointerFrameDuration);
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
      PlayUiSound(context_, hover_sfx_path_);
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
  LayoutUi(renderer);

  canvas_.Draw(renderer);
  DrawTitle(renderer);
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  DrawPointers(renderer);
}

void MainMenuScene::LayoutUi(engine::render::Renderer2D& renderer) {
  renderer.SetFont(std::string(constants::ui::kBodyFont));
  const auto window_size = context_.Window().GetSize();
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

    const float frame_pos =
        state.elapsed / constants::ui::MainMenu::kPointerFrameDuration;
    const std::size_t frame_index =
        std::min(frame_count - 1, static_cast<std::size_t>(frame_pos));
    auto texture = pointer_frames_[frame_index];
    if (!texture) continue;
    const auto tex_size = texture->GetSize();
    if (tex_size.y == 0) continue;

    const float scale = (constants::ui::MainMenu::kButtonHeight *
                         constants::ui::MainMenu::kPointerHeightFactor *
                         constants::ui::MainMenu::kPointerScaleFactor) /
                        static_cast<float>(tex_size.y);
    const float scaled_width = static_cast<float>(tex_size.x) * scale;
    const float scaled_height = static_cast<float>(tex_size.y) * scale;
    const auto pos = button->GetPosition();
    const auto size = button->GetSize();
    const float y = pos.y + (size.y - scaled_height) * 0.5f;
    const float left_x =
        pos.x - scaled_width - constants::ui::MainMenu::kPointerSpacing;
    const float right_x =
        pos.x + size.x + constants::ui::MainMenu::kPointerSpacing;

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
  const float draw_scale = scale * constants::ui::MainMenu::kTitleScaleFactor;
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
