#ifndef ENGINE_RENDER_CAMERA2D_H_
#define ENGINE_RENDER_CAMERA2D_H_

#include "engine/math/vector2.h"

namespace engine::render {

/**
 * @brief Simple 2D camera supporting translation and zoom.
 */
class Camera2D {
 public:
  Camera2D() = default;

  void SetPosition(const math::Vector2f& position) noexcept {
    position_ = position;
  }
  const math::Vector2f& GetPosition() const noexcept { return position_; }

  void Move(const math::Vector2f& delta) noexcept { position_ += delta; }

  void SetZoom(float zoom) noexcept;
  float GetZoom() const noexcept { return zoom_; }

  void ZoomBy(float delta) noexcept { SetZoom(zoom_ + delta); }

  /**
   * @brief Transform a world-space point into screen space taking parallax into
   * account.
   */
  math::Vector2f Apply(const math::Vector2f& point,
                       float parallax) const noexcept;

 private:
  math::Vector2f position_{0.0f, 0.0f};
  float zoom_{1.0f};

  static constexpr float kMinimumZoom = 0.05f;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_CAMERA2D_H_
