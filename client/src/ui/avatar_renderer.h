/**
 * @file avatar_renderer.h
 * @brief Utility for rendering player avatars from sprite sheet
 */

#ifndef CLIENT_UI_AVATAR_RENDERER_H_
#define CLIENT_UI_AVATAR_RENDERER_H_

#include <cstdint>
#include <memory>

#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"

namespace client {

class ClientAssetManager;

namespace ui {

/**
 * @brief Utility for rendering player avatars from a sprite sheet
 */
class AvatarRenderer {
 public:
  /**
   * @brief Construct an avatar renderer
   * @param assets Asset manager for loading textures
   */
  explicit AvatarRenderer(ClientAssetManager& assets);

  /**
   * @brief Draw an avatar at the specified position
   * @param renderer Renderer to use
   * @param avatar_index Index of the avatar (0-based)
   * @param position Top-left position
   * @param size Display size (width and height)
   */
  void Draw(engine::render::Renderer2D& renderer, std::uint8_t avatar_index,
            engine::math::Vector2f position, float size);

  /**
   * @brief Get the source rectangle for a given avatar index
   * @param avatar_index Index of the avatar (0-based)
   * @return Source rectangle in sprite sheet coordinates
   */
  static engine::math::RectF GetSourceRect(std::uint8_t avatar_index);

  /**
   * @brief Check if the texture is loaded
   * @return True if texture is available
   */
  bool IsLoaded() const { return texture_ != nullptr; }

 private:
  ClientAssetManager& assets_;
  std::shared_ptr<engine::render::Texture2D> texture_;
};

}  // namespace ui
}  // namespace client

#endif  // CLIENT_UI_AVATAR_RENDERER_H_
