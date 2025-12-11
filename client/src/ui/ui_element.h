#ifndef CLIENT_UI_UI_ELEMENT_H_
#define CLIENT_UI_UI_ELEMENT_H_

#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/input.h"
#include "engine/math/vector2.h"

namespace client::ui {

class UIElement {
 public:
  virtual ~UIElement() = default;

  virtual void Update(engine::time::TimeDelta dt, engine::input::InputManager& input) = 0;
  virtual void Draw(engine::render::Renderer2D& renderer) = 0;

  virtual void SetPosition(engine::math::Vector2f pos) = 0;
  virtual engine::math::Vector2f GetPosition() const = 0;
  
  virtual void SetSize(engine::math::Vector2f size) = 0;
  virtual engine::math::Vector2f GetSize() const = 0;
};

}  // namespace client::ui

#endif  // CLIENT_UI_UI_ELEMENT_H_
