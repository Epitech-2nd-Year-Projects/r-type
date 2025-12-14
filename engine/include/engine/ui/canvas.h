/**
 * @file canvas.h
 * @brief UI canvas owning a layout tree
 *
 * @details
 * Canvas drives measurement and arrangement of UI elements for a given
 * viewport, then submits draw calls using the configured renderer.
 */

#ifndef ENGINE_UI_CANVAS_H_
#define ENGINE_UI_CANVAS_H_

#include <memory>

#include "engine/ui/element.h"

namespace engine::ui {

/**
 * @brief Root container responsible for laying out and drawing UI elements
 */
class Canvas {
 public:
  Canvas() = default;

  explicit Canvas(const math::Vector2f& viewport) : viewport_size_(viewport) {}

  /**
   * @brief Update the viewport size used for layout
   */
  void SetViewportSize(const math::Vector2f& viewport) {
    viewport_size_ = viewport;
  }

  /**
   * @brief Define the layout tree root
   */
  void SetRoot(std::shared_ptr<UIElement> root) { root_ = std::move(root); }

  /**
   * @brief Access the current root element
   */
  std::shared_ptr<UIElement> Root() const { return root_; }

  /**
   * @brief Measure and arrange the UI tree
   */
  void Layout(render::Renderer2D& renderer);

  /**
   * @brief Draw the previously arranged UI tree
   */
  void Draw(render::Renderer2D& renderer) const;

  /**
   * @brief Convenience helper to layout and draw in one call
   */
  void LayoutAndDraw(render::Renderer2D& renderer) {
    Layout(renderer);
    Draw(renderer);
  }

 private:
  LayoutContext BuildContext(render::Renderer2D& renderer) const;

  std::shared_ptr<UIElement> root_{};
  math::Vector2f viewport_size_{0.0f, 0.0f};
};

}  // namespace engine::ui

#endif  // ENGINE_UI_CANVAS_H_
