/**
 * @file layouts.h
 * @brief Layout containers used to arrange UI elements
 *
 * @details
 * Provides stack based and anchor based layouts that adapt to viewport changes
 * and center or align children without hardcoded coordinates.
 */

#ifndef ENGINE_UI_LAYOUTS_H_
#define ENGINE_UI_LAYOUTS_H_

#include <vector>

#include "engine/ui/element.h"

namespace engine::ui {

/**
 * @brief Alignment along the primary axis of a stack
 */
enum class StackAlignment { kStart, kCenter, kEnd };

/**
 * @brief Linear layout that stacks children horizontally or vertically
 */
class StackContainer : public UIContainer {
 public:
  explicit StackContainer(Axis axis);

  /**
   * @brief Set spacing between stacked children
   */
  void SetSpacing(float spacing) { spacing_ = spacing; }

  /**
   * @brief Apply padding inside the container frame
   */
  void SetPadding(const Insets& padding) { padding_ = padding; }

  /**
   * @brief Configure alignment along the stacking axis
   */
  void SetMainAlignment(StackAlignment alignment) {
    main_alignment_ = alignment;
  }

  /**
   * @brief Configure alignment on the perpendicular axis
   */
  void SetChildAlignment(const Alignment2D& alignment) {
    child_alignment_ = alignment;
    use_child_alignment_ = true;
  }

 protected:
  math::Vector2f ComputeContentSize(
      const LayoutContext& context,
      const math::Vector2f& available_space) override;

  void OnLayoutUpdated(const LayoutContext& context) override;

 private:
  void ArrangeVertical(const LayoutContext& context,
                       const math::RectF& content_bounds);
  void ArrangeHorizontal(const LayoutContext& context,
                         const math::RectF& content_bounds);

  Axis axis_;
  float spacing_{0.0f};
  Insets padding_{};
  StackAlignment main_alignment_{StackAlignment::kStart};
  Alignment2D child_alignment_{};
  bool use_child_alignment_{false};
};

/**
 * @brief Placement configuration for anchored children
 */
struct AnchoredChild {
  std::shared_ptr<UIElement> element;
  Anchor anchor{Anchor::TopLeft()};
  math::Vector2f offset{};
};

/**
 * @brief Layout that positions children relative to normalized anchors
 */
class AnchorContainer : public UIContainer {
 public:
  AnchorContainer() = default;

  /**
   * @brief Add a child at a specific anchor with an optional offset
   */
  void AddAnchoredChild(const std::shared_ptr<UIElement>& child, Anchor anchor,
                        const math::Vector2f& offset = {});

  /**
   * @brief Set padding to keep anchored content away from edges
   */
  void SetPadding(const Insets& padding) { padding_ = padding; }

 protected:
  math::Vector2f ComputeContentSize(
      const LayoutContext& /*context*/,
      const math::Vector2f& available_space) override;

  void OnLayoutUpdated(const LayoutContext& context) override;

 private:
  Insets padding_{};
  std::vector<AnchoredChild> anchored_children_{};
};

}  // namespace engine::ui

#endif  // ENGINE_UI_LAYOUTS_H_
