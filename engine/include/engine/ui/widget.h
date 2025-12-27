/**
 * @file widget_h
 * @brief Interactive widget base for UI
 *
 * @details
 * Provides update and draw hooks plus manual position and size access
 */

#ifndef ENGINE_UI_WIDGET_H_
#define ENGINE_UI_WIDGET_H_

#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

namespace engine::ui {

/**
 * @brief Base class for interactive widgets
 */
class Widget {
 public:
  virtual ~Widget() = default;

  /**
   * @brief Update widget state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  virtual void Update(engine::time::TimeDelta dt,
                      engine::input::InputManager& input) = 0;

  /**
   * @brief Draw widget visuals
   * @param renderer Renderer instance
   */
  virtual void Draw(engine::render::Renderer2D& renderer) = 0;

  /**
   * @brief Set widget position
   * @param pos Position in screen space
   */
  virtual void SetPosition(engine::math::Vector2f pos) = 0;

  /**
   * @brief Get widget position
   * @return Position in screen space
   */
  virtual engine::math::Vector2f GetPosition() const = 0;

  /**
   * @brief Set widget size
   * @param size Size in screen space
   */
  virtual void SetSize(engine::math::Vector2f size) = 0;

  /**
   * @brief Get widget size
   * @return Size in screen space
   */
  virtual engine::math::Vector2f GetSize() const = 0;
};

}  // namespace engine::ui

#endif  // ENGINE_UI_WIDGET_H_
