/**
 * @file render_queue_system.h
 * @brief Build and submit ordered sprite draw commands
 */

#ifndef CLIENT_ECS_RENDER_QUEUE_SYSTEM_H_
#define CLIENT_ECS_RENDER_QUEUE_SYSTEM_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/archetype_registry.h"
#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/render/renderer2d.h"

namespace client::ecs {

/**
 * @class RenderQueueSystem
 * @brief Builds and renders a sorted sprite draw queue
 */
class RenderQueueSystem {
 public:
  /**
   * @brief Create a render queue system
   * @param registry ECS registry reference
   * @param renderer Renderer used to draw sprites
   */
  RenderQueueSystem(engine::ecs::Registry& registry,
                    engine::render::Renderer2D& renderer);

  /**
   * @brief Clear cached textures and draw queue
   */
  void Reset();

  /**
   * @brief Build and render draw commands for visible sprites
   */
  void Render();

 private:
  struct DrawCommand {
    std::shared_ptr<engine::render::Texture2D> texture;
    engine::render::SpriteDrawParams params;
    std::int32_t layer{0};
    float depth{0.0f};
    std::size_t entity_index{0};
  };

  void RegisterComponents();
  engine::render::SpriteDrawParams BuildParams(
      const ecs::PositionComponent& position,
      const ecs::SpriteComponent& sprite,
      const ecs::RenderLayerComponent& layer,
      const std::optional<ecs::VelocityComponent>& velocity,
      std::uint16_t type_code,
      const std::shared_ptr<engine::render::Texture2D>& texture,
      const std::string& texture_id) const;
  engine::math::RectF ApplyFlip(const engine::math::RectF& base, bool flip_x,
                                bool flip_y) const;
  engine::math::Vector2f ComputeScale(const engine::render::Texture2D& texture,
                                      const engine::math::RectF& source) const;
  bool ComputeFlipX(std::uint16_t type_code,
                    const std::optional<ecs::VelocityComponent>& velocity,
                    bool sprite_flip, const std::string& texture_id) const;
  std::shared_ptr<engine::render::Texture2D> LoadTexture(const std::string& id);

  engine::ecs::Registry& registry_;
  engine::render::Renderer2D& renderer_;
  const ArchetypeRegistry& archetypes_;
  std::unordered_map<std::string, std::shared_ptr<engine::render::Texture2D>>
      textures_;
  std::vector<DrawCommand> draw_queue_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_RENDER_QUEUE_SYSTEM_H_
