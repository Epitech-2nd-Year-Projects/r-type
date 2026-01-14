/**
 * @file avatar_renderer.cpp
 * @brief Avatar renderer implementation
 */

#include "avatar_renderer.h"

#include <algorithm>

#include "client_asset_manager.h"
#include "constants/ui_constants.h"

namespace client::ui {

AvatarRenderer::AvatarRenderer(ClientAssetManager& assets) : assets_(assets) {
  texture_ = assets_.GetTexture(constants::ui::Profile::kAvatarSpritePath);
}

void AvatarRenderer::Draw(engine::render::Renderer2D& renderer,
                          std::uint8_t avatar_index,
                          engine::math::Vector2f position, float size) {
  if (!texture_) {
    return;
  }

  const auto max_index =
      static_cast<std::uint8_t>(constants::ui::Profile::kAvatarCount - 1);
  if (avatar_index > max_index) {
    avatar_index = max_index;
  }

  engine::render::SpriteDrawParams params;
  params.source = GetSourceRect(avatar_index);

  const float sprite_width =
      static_cast<float>(constants::ui::Profile::kAvatarSpriteWidth);
  const float sprite_height =
      static_cast<float>(constants::ui::Profile::kAvatarSpriteHeight);
  const float scale_factor = size / std::max(sprite_width, sprite_height);
  params.scale = {scale_factor, scale_factor};

  const float rendered_width = sprite_width * scale_factor;
  const float rendered_height = sprite_height * scale_factor;

  const float offset_x = (size - rendered_width) / 2.0f;
  const float offset_y = (size - rendered_height) / 2.0f;
  params.position = {
      position.x + offset_x + constants::ui::Profile::kAvatarOffsetX,
      position.y + offset_y + constants::ui::Profile::kAvatarOffsetY};

  renderer.DrawTexture(*texture_, params);
}

engine::math::RectF AvatarRenderer::GetSourceRect(std::uint8_t avatar_index) {
  const int col = avatar_index % constants::ui::Profile::kAvatarColumns;
  const int row = avatar_index / constants::ui::Profile::kAvatarColumns;

  return engine::math::RectF{
      static_cast<float>(col * constants::ui::Profile::kAvatarSpriteWidth),
      static_cast<float>(row * constants::ui::Profile::kAvatarSpriteHeight),
      static_cast<float>(constants::ui::Profile::kAvatarSpriteWidth),
      static_cast<float>(constants::ui::Profile::kAvatarSpriteHeight)};
}

}  // namespace client::ui
