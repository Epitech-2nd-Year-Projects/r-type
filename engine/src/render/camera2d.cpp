#include "engine/render/camera2d.h"

#include <algorithm>

namespace engine::render {

void Camera2D::SetZoom(float zoom) noexcept {
  zoom_ = std::max(zoom, kMinimumZoom);
}

math::Vector2f Camera2D::Apply(const math::Vector2f& point,
                               float parallax) const noexcept {
  return (point - (position_ * parallax)) * zoom_;
}

}  // namespace engine::render
