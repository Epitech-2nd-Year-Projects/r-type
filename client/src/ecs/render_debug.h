/**
 * @file render_debug.h
 * @brief Render debug overlays for the client ECS world
 */

#ifndef CLIENT_ECS_RENDER_DEBUG_H_
#define CLIENT_ECS_RENDER_DEBUG_H_

#include "ecs/components.h"
#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/registry.h"
#include "engine/render/renderer2d.h"

namespace client::ecs {

/**
 * @class RenderDebug
 * @brief Draws debug overlays such as hitboxes
 */
class RenderDebug {
 public:
  /**
   * @brief Create a render debug helper
   * @param registry ECS registry reference
   * @param renderer Renderer used for overlays
   */
  RenderDebug(engine::ecs::Registry& registry,
              engine::render::Renderer2D& renderer);

  /**
   * @brief Enable or disable debug rendering
   * @param enabled Toggle state
   */
  void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }

  /**
   * @brief Query current debug state
   * @return True when debug rendering is enabled
   */
  bool enabled() const { return enabled_; }

  /**
   * @brief Draw all enabled debug overlays
   */
  void Draw();

 private:
  void RegisterComponents();

  engine::ecs::Registry& registry_;
  engine::render::Renderer2D& renderer_;
  bool enabled_{false};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_RENDER_DEBUG_H_
