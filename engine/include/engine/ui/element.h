/**
 * @file element.h
 * @brief Core UI element abstractions
 *
 * @details
 * Provides the base UIElement contract and shared container logic used by the
 * layout system. Elements support measurement, arrangement and rendering while
 * honoring layout properties and margins.
 */

#ifndef ENGINE_UI_ELEMENT_H_
#define ENGINE_UI_ELEMENT_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "engine/math/rect.h"
#include "engine/render/color.h"
#include "engine/ui/types.h"

namespace engine::ui {

/**
 * @brief Base class for all UI elements participating in layout
 */
class UIElement {
 public:
  virtual ~UIElement() = default;

  /**
   * @brief Measure desired size within an available space
   */
  math::Vector2f Measure(const LayoutContext& context,
                         const math::Vector2f& available_space);

  /**
   * @brief Arrange the element inside the provided bounds
   */
  void Arrange(const LayoutContext& context, const math::RectF& bounds);

  /**
   * @brief Render the element
   */
  virtual void Draw(render::Renderer2D& renderer) const = 0;

  /**
   * @brief Access the final frame after layout
   */
  const math::RectF& Frame() const { return frame_; }

  /**
   * @brief Access layout properties
   */
  LayoutProperties& Layout() { return layout_; }

  /**
   * @brief Access immutable layout properties
   */
  const LayoutProperties& Layout() const { return layout_; }

  /**
   * @brief Override all layout properties in one call
   */
  void SetLayout(const LayoutProperties& props) {
    layout_ = props;
    measured_ = false;
  }

  /**
   * @brief Mark cached measurements as invalid
   */
  void InvalidateMeasure() { measured_ = false; }

  /**
   * @brief Last measured content size excluding margins
   */
  math::Vector2f MeasuredContentSize() const { return measured_content_size_; }

  /**
   * @brief Last measured size including margins
   */
  math::Vector2f MeasuredSizeWithMargin() const {
    return measured_with_margin_;
  }

 protected:
  /**
   * @brief Compute content size within available space
   */
  virtual math::Vector2f ComputeContentSize(
      const LayoutContext& context, const math::Vector2f& available_space) = 0;

  /**
   * @brief Notification hook after layout updates
   */
  virtual void OnLayoutUpdated(const LayoutContext& /*context*/) {}

  /**
   * @brief Rectangle representing the content area
   */
  math::RectF ContentRect() const { return frame_; }

 private:
  float ResolveSizeForAxis(const LayoutValue& value, float available,
                           float content, bool stretch) const;
  float AlignOffset(float available, float size,
                    HorizontalAlignment alignment) const;
  float AlignOffset(float available, float size,
                    VerticalAlignment alignment) const;

  LayoutProperties layout_{};
  math::RectF frame_{};
  math::Vector2f measured_content_size_{};
  math::Vector2f measured_with_margin_{};
  bool measured_{false};
};

/**
 * @brief Shared container logic for elements owning children
 */
class UIContainer : public UIElement {
 public:
  /**
   * @brief Add a child element
   */
  void AddChild(std::shared_ptr<UIElement> child);

  /**
   * @brief Access immutable children
   */
  const std::vector<std::shared_ptr<UIElement>>& Children() const {
    return children_;
  }

 protected:
  void Draw(render::Renderer2D& renderer) const override;

  /**
   * @brief Hook for derived containers to render their own visuals
   */
  virtual void DrawContent(render::Renderer2D& renderer) const;

  std::vector<std::shared_ptr<UIElement>>& Children() { return children_; }

 private:
  std::vector<std::shared_ptr<UIElement>> children_{};
};

/**
 * @brief Simple box element useful as spacer, panel or layout anchor
 */
class BoxElement : public UIElement {
 public:
  /**
   * @brief Optional background fill color
   */
  void SetBackground(render::Color color);

  /**
   * @brief Register a callback invoked when layout is updated
   */
  void SetLayoutCallback(std::function<void(const math::RectF&)> callback);

 protected:
  math::Vector2f ComputeContentSize(
      const LayoutContext& /*context*/,
      const math::Vector2f& available_space) override;

  void OnLayoutUpdated(const LayoutContext& context) override;

  void Draw(render::Renderer2D& renderer) const override;

 private:
  /**
   * @brief Optional background fill color
   */
  std::optional<render::Color> background_{};
  /**
   * @brief Callback invoked after layout updates with the new frame
   */
  std::function<void(const math::RectF&)> layout_callback_{};
};

}  // namespace engine::ui

#endif  // ENGINE_UI_ELEMENT_H_
