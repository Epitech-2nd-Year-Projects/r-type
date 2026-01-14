#include "ui/menu_background.h"

#include <algorithm>

namespace client::ui {

MenuBackground::MenuBackground(std::string_view video_path)
    : video_path_(video_path) {
  if (video_path_.empty()) {
    return;
  }
  media_ =
      LoadMediaEx(video_path_.c_str(), MEDIA_LOAD_NO_AUDIO | MEDIA_FLAG_LOOP);
  media_loaded_ = IsMediaValid(media_);
}

MenuBackground::~MenuBackground() {
  if (media_loaded_) {
    UnloadMedia(&media_);
  }
}

void MenuBackground::Update(engine::time::TimeDelta dt) {
  if (!media_loaded_) {
    return;
  }
  UpdateMediaEx(&media_, static_cast<double>(dt.as_seconds()));
}

void MenuBackground::Draw(const engine::math::Vector2i& size) const {
  Draw(size, 1.0f);
}

void MenuBackground::Draw(const engine::math::Vector2i& size,
                          float alpha) const {
  if (!media_loaded_) {
    return;
  }
  if (media_.videoTexture.id == 0) {
    return;
  }
  const float window_width = static_cast<float>(size.x);
  const float window_height = static_cast<float>(size.y);
  if (window_width <= 0.0f || window_height <= 0.0f) {
    return;
  }
  const float texture_width = static_cast<float>(media_.videoTexture.width);
  const float texture_height = static_cast<float>(media_.videoTexture.height);
  if (texture_width <= 0.0f || texture_height <= 0.0f) {
    return;
  }

  Rectangle source{0.0f, 0.0f, texture_width, texture_height};
  const float window_ratio = window_width / window_height;
  const float texture_ratio = texture_width / texture_height;
  if (texture_ratio > window_ratio) {
    const float crop_width = texture_height * window_ratio;
    source.x = (texture_width - crop_width) * 0.5f;
    source.width = crop_width;
  } else if (texture_ratio < window_ratio) {
    const float crop_height = texture_width / window_ratio;
    source.y = (texture_height - crop_height) * 0.5f;
    source.height = crop_height;
  }

  Rectangle dest{0.0f, 0.0f, window_width, window_height};
  const float clamped_alpha = std::clamp(alpha, 0.0f, 1.0f);
  Color tint = WHITE;
  tint.a = static_cast<unsigned char>(clamped_alpha * 255.0f);
  DrawTexturePro(media_.videoTexture, source, dest, {0.0f, 0.0f}, 0.0f, tint);
}

}  // namespace client::ui
