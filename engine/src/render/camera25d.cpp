#include "engine/render/camera25d.h"

#include <algorithm>

namespace engine::render {

namespace {

float ClampExtent(float value, float minimum) noexcept {
  return std::max(value, minimum);
}

}  // namespace

Camera25D::Camera25D() noexcept { UpdatePixelsPerUnit(); }

Camera25D::Camera25D(const math::Vector2f& viewport_size, float vertical_min,
                     float vertical_max) noexcept
    : viewport_size_(viewport_size),
      vertical_min_(vertical_min),
      vertical_max_(
          std::max(vertical_max, vertical_min + kMinimumWorldHeight)) {
  viewport_size_.x = std::max(viewport_size_.x, kMinimumViewportExtent);
  viewport_size_.y = std::max(viewport_size_.y, kMinimumViewportExtent);
  UpdatePixelsPerUnit();
}

void Camera25D::SetViewportSize(const math::Vector2f& size) noexcept {
  viewport_size_.x = std::max(size.x, kMinimumViewportExtent);
  viewport_size_.y = std::max(size.y, kMinimumViewportExtent);
  UpdatePixelsPerUnit();
}

void Camera25D::SetVerticalRange(float min_y, float max_y) noexcept {
  vertical_min_ = min_y;
  vertical_max_ = std::max(max_y, min_y + kMinimumWorldHeight);
  UpdatePixelsPerUnit();
}

math::Vector2f Camera25D::WorldToScreen(const math::Vector2f& world,
                                        float parallax) const noexcept {
  const math::Vector2f view_size = GetViewSizeWorld();
  const float half_width = view_size.x * 0.5f;

  const math::Vector2f origin{focus_x_ - half_width, vertical_min_};
  const math::Vector2f parallax_origin = origin * parallax;

  math::Vector2f screen = (world - parallax_origin) * pixels_per_unit_;
  return screen;
}

math::Vector2f Camera25D::WorldToScreen(const math::Vector2f& world,
                                        RenderLayer layer) const noexcept {
  return WorldToScreen(world, GetLayerParallax(layer));
}

math::Vector2f Camera25D::GetViewSizeWorld() const noexcept {
  const float world_height =
      ClampExtent(vertical_max_ - vertical_min_, kMinimumWorldHeight);
  const float ppu = pixels_per_unit_ <= 0.0f ? 1.0f : pixels_per_unit_;
  return {viewport_size_.x / ppu, world_height};
}

math::RectF Camera25D::GetViewRectWorld() const noexcept {
  const math::Vector2f view_size = GetViewSizeWorld();
  const float half_width = view_size.x * 0.5f;
  const float left = focus_x_ - half_width;
  return {left, vertical_min_, view_size.x, view_size.y};
}

void Camera25D::UpdatePixelsPerUnit() noexcept {
  const float world_height =
      ClampExtent(vertical_max_ - vertical_min_, kMinimumWorldHeight);
  pixels_per_unit_ = viewport_size_.y / world_height;
  if (pixels_per_unit_ <= 0.0f) {
    pixels_per_unit_ = 1.0f;
  }
}

}  // namespace engine::render
