/**
 * @file types.h
 * @brief Fundamental UI layout primitives
 *
 * @details
 * Shared layout structures used by the UI system including sizing policies,
 * alignment rules, anchors and font scaling helpers
 */

#ifndef ENGINE_UI_TYPES_H_
#define ENGINE_UI_TYPES_H_

#include "engine/math/vector2.h"

namespace engine::render {
class Renderer2D;
}  // namespace engine::render

namespace engine::ui {

/**
 * @brief Axis used by linear layouts
 */
enum class Axis { kHorizontal, kVertical };

/**
 * @brief Unit used to resolve layout values
 */
enum class LayoutUnit { kAuto, kPixels, kPercent };

/**
 * @brief Declarative layout value supporting automatic, pixel or percentage
 * sizing
 */
class LayoutValue {
 public:
  /**
   * @brief Create an automatic layout value
   */
  static LayoutValue Auto();

  /**
   * @brief Create a pixel based layout value
   */
  static LayoutValue Pixels(float value);

  /**
   * @brief Create a percentage based layout value
   */
  static LayoutValue Percent(float value);

  /**
   * @brief Access the underlying unit
   */
  LayoutUnit unit() const { return unit_; }

  /**
   * @brief Stored numeric payload for the value
   */
  float value() const { return value_; }

 private:
  LayoutValue(LayoutUnit unit, float value);

  LayoutUnit unit_{LayoutUnit::kAuto};
  float value_{0.0f};
};

/**
 * @brief Convenience wrapper bundling width and height layout values
 */
struct LayoutSize {
  LayoutValue width{LayoutValue::Auto()};
  LayoutValue height{LayoutValue::Auto()};
};

/**
 * @brief Margins or padding expressed per side
 */
struct Insets {
  float left{0.0f};
  float right{0.0f};
  float top{0.0f};
  float bottom{0.0f};

  /**
   * @brief Create uniform insets
   */
  static Insets Uniform(float value);

  /**
   * @brief Combined horizontal extent
   */
  float Horizontal() const { return left + right; }

  /**
   * @brief Combined vertical extent
   */
  float Vertical() const { return top + bottom; }
};

/**
 * @brief Horizontal alignment when space remains after measurement
 */
enum class HorizontalAlignment { kStart, kCenter, kEnd, kStretch };

/**
 * @brief Vertical alignment when space remains after measurement
 */
enum class VerticalAlignment { kStart, kCenter, kEnd, kStretch };

/**
 * @brief Pair of horizontal and vertical alignment settings
 */
struct Alignment2D {
  HorizontalAlignment horizontal{HorizontalAlignment::kStart};
  VerticalAlignment vertical{VerticalAlignment::kStart};
};

/**
 * @brief Normalized anchor used to attach elements relative to a parent
 */
struct Anchor {
  float x{0.0f};
  float y{0.0f};

  /**
   * @brief Convenience anchor at the top left corner
   */
  static Anchor TopLeft();

  /**
   * @brief Convenience anchor at the center of the parent
   */
  static Anchor Center();

  /**
   * @brief Convenience anchor at the bottom right corner
   */
  static Anchor BottomRight();
};

/**
 * @brief Unit used to resolve font sizes relative to the viewport
 */
enum class FontSizeUnit {
  kPixels,
  kViewportWidth,
  kViewportHeight,
  kViewportMin
};

/**
 * @brief Font sizing helper supporting pixel and viewport relative scales
 */
class FontSize {
 public:
  FontSize();

  /**
   * @brief Create a fixed pixel font size
   */
  static FontSize Pixels(float value);

  /**
   * @brief Create a font size relative to the viewport width
   */
  static FontSize RelativeWidth(float ratio);

  /**
   * @brief Create a font size relative to the viewport height
   */
  static FontSize RelativeHeight(float ratio);

  /**
   * @brief Create a font size relative to the smallest viewport dimension
   */
  static FontSize RelativeMin(float ratio);

  /**
   * @brief Resolve to a concrete pixel size given a viewport
   */
  float Resolve(const math::Vector2f& viewport) const;

  /**
   * @brief Access the configured unit
   */
  FontSizeUnit unit() const { return unit_; }

  /**
   * @brief Access the stored value
   */
  float value() const { return value_; }

 private:
  FontSizeUnit unit_;
  float value_;
};

/**
 * @brief Shared layout configuration carried by UI elements
 */
struct LayoutProperties {
  LayoutSize size{};
  Insets margin{};
  Alignment2D alignment{};
  math::Vector2f pivot{0.5f, 0.5f};
};

/**
 * @brief Context passed to layout routines with renderer access
 */
struct LayoutContext {
  render::Renderer2D& renderer;
  math::Vector2f viewport_size;
};

}  // namespace engine::ui

#endif  // ENGINE_UI_TYPES_H_
