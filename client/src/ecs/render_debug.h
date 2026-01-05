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

  bool show_colliders{false};
  bool show_sprite_bounds{false};
  bool show_velocity{false};
  bool show_ai_paths{false};

  void DrawLine(const engine::math::Vector2f& start,
                const engine::math::Vector2f& end,
                const engine::render::Color& color =
                    engine::render::Color(1.0f, 0.0f, 0.0f));

  void DrawPolyline(const std::vector<engine::math::Vector2f>& points,
                    const engine::render::Color& color =
                        engine::render::Color(0.0f, 1.0f, 0.0f));

  void DrawAnimatedPolyline(const std::vector<engine::math::Vector2f>& points,
                            const engine::render::Color& color =
                                engine::render::Color(0.0f, 1.0f, 0.0f),
                            float total_time_seconds = 0.0f);
  bool enabled_{false};

 private:
  void RegisterComponents();

  engine::ecs::Registry& registry_;
  engine::render::Renderer2D& renderer_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_RENDER_DEBUG_H_
