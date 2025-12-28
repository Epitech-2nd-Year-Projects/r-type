/**
 * @file render_system.h
 * @brief ECS driven sprite rendering coordinator
 */

#ifndef CLIENT_ECS_RENDER_SYSTEM_H_
#define CLIENT_ECS_RENDER_SYSTEM_H_

#include "ecs/render_queue_system.h"
#include "ecs/sprite_sync_system.h"

namespace client::ecs {

/**
 * @class RenderSystem
 * @brief Runs sprite sync and render queue phases
 */
class RenderSystem {
 public:
  /**
   * @brief Create a render system
   * @param registry ECS registry reference
   * @param renderer Renderer used for sprites
   */
  RenderSystem(engine::ecs::Registry& registry,
               engine::render::Renderer2D& renderer);

  /**
   * @brief Clear cached textures and draw queue
   */
  void Reset();

  /**
   * @brief Sync sprites and render the draw queue
   */
  void Render();

 private:
  SpriteSyncSystem sprite_sync_;
  RenderQueueSystem render_queue_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_RENDER_SYSTEM_H_
