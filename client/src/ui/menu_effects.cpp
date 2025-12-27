#include "ui/menu_effects.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

#include "client_asset_manager.h"
#include "client_context.h"
#include "engine/audio/audio_engine.h"
#include "engine/math/rect.h"

namespace client::ui {

MenuEffects::MenuEffects(ClientContext& context, MenuPointerConfig config,
                         std::string_view hover_sfx_path,
                         std::string_view click_sfx_path)
    : context_(context), config_(config) {
  if (!hover_sfx_path.empty()) {
    hover_sfx_path_ = context_.Assets().GetSfxPath(hover_sfx_path);
  }
  if (!click_sfx_path.empty()) {
    click_sfx_path_ = context_.Assets().GetSfxPath(click_sfx_path);
  }
}

void MenuEffects::Load() {
  pointer_frames_.clear();
  if (config_.frame_count <= 0) {
    return;
  }
  pointer_frames_.reserve(static_cast<std::size_t>(config_.frame_count));
  for (int i = 0; i < config_.frame_count; ++i) {
    std::ostringstream path;
    path << config_.frame_prefix << std::setw(4) << std::setfill('0') << i
         << config_.frame_extension;
    auto tex = context_.Assets().GetTexture(path.str());
    if (tex) {
      pointer_frames_.push_back(tex);
    }
  }
}

void MenuEffects::Update(
    engine::time::TimeDelta dt, engine::input::InputManager& input,
    std::span<const std::shared_ptr<engine::ui::Button>> buttons) {
  EnsureStateCount(buttons.size());
  const auto mouse_pos = input.GetMousePosition();
  const float max_elapsed =
      pointer_frames_.empty()
          ? 0.0f
          : (static_cast<float>(pointer_frames_.size() - 1) *
             config_.frame_duration);

  for (std::size_t i = 0; i < buttons.size(); ++i) {
    const auto& button = buttons[i];
    auto& state = pointer_states_[i];
    state.was_hovered = state.hovered;
    state.hovered =
        button &&
        engine::math::RectF{button->GetPosition(), button->GetSize()}.Contains(
            mouse_pos);
    if (state.hovered && !state.was_hovered) {
      state.elapsed = 0.0f;
      state.animating = true;
      PlaySound(hover_sfx_path_);
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
}

void MenuEffects::DrawPointers(
    engine::render::Renderer2D& renderer,
    std::span<const std::shared_ptr<engine::ui::Button>> buttons) const {
  if (pointer_frames_.empty() || config_.frame_duration <= 0.0f) {
    return;
  }
  const std::size_t frame_count = pointer_frames_.size();
  const std::size_t count = std::min(pointer_states_.size(), buttons.size());
  for (std::size_t i = 0; i < count; ++i) {
    const auto& button = buttons[i];
    const auto& state = pointer_states_[i];
    if (!button || !state.hovered) {
      continue;
    }

    const float frame_pos = state.elapsed / config_.frame_duration;
    const std::size_t frame_index =
        std::min(frame_count - 1, static_cast<std::size_t>(frame_pos));
    auto texture = pointer_frames_[frame_index];
    if (!texture) {
      continue;
    }
    const auto tex_size = texture->GetSize();
    if (tex_size.y == 0) {
      continue;
    }

    const auto size = button->GetSize();
    if (size.y <= 0.0f) {
      continue;
    }
    const float scale =
        (size.y * config_.height_factor * config_.scale_factor) /
        static_cast<float>(tex_size.y);
    const float scaled_width = static_cast<float>(tex_size.x) * scale;
    const float scaled_height = static_cast<float>(tex_size.y) * scale;
    const auto pos = button->GetPosition();
    const float y = pos.y + (size.y - scaled_height) * 0.5f;
    const float left_x = pos.x - scaled_width - config_.spacing;
    const float right_x = pos.x + size.x + config_.spacing;

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

std::function<void()> MenuEffects::WrapClick(
    std::function<void()> action) const {
  return [this, action = std::move(action)]() mutable {
    PlaySound(click_sfx_path_);
    if (action) {
      action();
    }
  };
}

void MenuEffects::PlaySound(const std::string& path) const {
  if (path.empty()) {
    return;
  }
  if (auto audio = context_.Audio()) {
    audio->PlaySoundEffect(path);
  }
}

void MenuEffects::EnsureStateCount(std::size_t count) {
  if (pointer_states_.size() != count) {
    pointer_states_.assign(count, PointerState{});
  }
}

}  // namespace client::ui
