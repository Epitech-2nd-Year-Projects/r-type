#ifndef ENGINE_RENDER_CAMERA25D_H_
#define ENGINE_RENDER_CAMERA25D_H_

#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/layer.h"

namespace engine::render {

/**
 * @class Camera25D
 * @brief Side-scrolling camera mapping world space to screen pixels
 *
 * @details
 * Maintains a fixed vertical range and converts world positions into screen
 * space using the active viewport size and per-layer parallax factors
 */
class Camera25D {
 public:
  /**
   * @brief Construct a camera with a 1x1 viewport and unit world height
   */
  Camera25D() noexcept;
  Camera25D(const math::Vector2f& viewport_size, float vertical_min,
            float vertical_max) noexcept;

  /**
   * @brief Set viewport size in pixels used for projections
   */
  void SetViewportSize(const math::Vector2f& size) noexcept;
  const math::Vector2f& GetViewportSize() const noexcept {
    return viewport_size_;
  }

  /**
   * @brief Define visible vertical world interval
   */
  void SetVerticalRange(float min_y, float max_y) noexcept;
  float GetVerticalMin() const noexcept { return vertical_min_; }
  float GetVerticalMax() const noexcept { return vertical_max_; }

  /**
   * @brief Update camera horizontal focus point
   */
  void SetFocusX(float focus_x) noexcept { focus_x_ = focus_x; }
  float GetFocusX() const noexcept { return focus_x_; }

  /**
   * @brief Convert a world position to screen coordinates using explicit
   * parallax
   */
  math::Vector2f WorldToScreen(const math::Vector2f& world,
                               float parallax) const noexcept;

  /**
   * @brief Convert a world position using render layer parallax
   */
  math::Vector2f WorldToScreen(const math::Vector2f& world,
                               RenderLayer layer) const noexcept;

  /**
   * @brief Pixels produced per world unit in the current view
   */
  float GetPixelsPerUnit() const noexcept { return pixels_per_unit_; }

  /**
   * @brief World dimensions represented by the current viewport
   */
  math::Vector2f GetViewSizeWorld() const noexcept;

  /**
   * @brief Visible world rectangle for the midground layer
   */
  math::RectF GetViewRectWorld() const noexcept;

 private:
  void UpdatePixelsPerUnit() noexcept;

  math::Vector2f viewport_size_{1.0f, 1.0f};
  float focus_x_{0.0f};
  float vertical_min_{0.0f};
  float vertical_max_{1.0f};
  float pixels_per_unit_{1.0f};

  static constexpr float kMinimumViewportExtent = 1.0f;
  static constexpr float kMinimumWorldHeight = 1.0f;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_CAMERA25D_H_
