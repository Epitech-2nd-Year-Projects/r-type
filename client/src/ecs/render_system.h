/**
 * @file render_system.h
 * @brief ECS-driven sprite rendering for the client world
 */

#ifndef CLIENT_ECS_RENDER_SYSTEM_H_
#define CLIENT_ECS_RENDER_SYSTEM_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/render/renderer2d.h"

namespace client::ecs {

/**
 * @class RenderSystem
 * @brief Translates ECS components into ordered draw calls
 *
 * Maintains sprite definitions per networked entity type, loads textures on
 * demand, and submits draw commands sorted by layer and depth.
 */
class RenderSystem {
 public:
  struct SpriteDefinition {
    std::string texture_id;
    engine::math::RectF source_rect;
    std::int32_t layer{0};
    float depth{0.0f};
    bool face_left{false};
    engine::render::Color tint{engine::render::Color::White()};
  };

  struct DrawCommand {
    std::shared_ptr<engine::render::Texture2D> texture;
    engine::render::SpriteDrawParams params;
    std::int32_t layer{0};
    float depth{0.0f};
    std::size_t entity_index{0};
  };

  RenderSystem(engine::ecs::Registry &registry,
               engine::render::Renderer2D &renderer);

  /**
   * @brief Clear cached textures and draw queue
   */
  void Reset();

  /**
   * @brief Populate sprite components and emit draw calls
   */
  void Render();

  void SetDebugHitboxes(bool enabled) noexcept { debug_hitboxes_ = enabled; }

 private:
  void RegisterComponents();
  void SyncSprite(std::size_t index, const SpriteDefinition &definition,
                  const std::optional<ecs::HealthComponent> &health,
                  const std::optional<ecs::VelocityComponent> &velocity);
  std::optional<SpriteDefinition> ResolveDefinition(
      const ecs::NetworkedEntityComponent &net,
      const std::optional<ecs::HealthComponent> &health,
      const std::optional<ecs::VelocityComponent> &velocity,
      std::size_t entity_index) const;
  SpriteDefinition ResolveEnemy(
      const std::optional<ecs::HealthComponent> &health,
      const std::optional<ecs::VelocityComponent> &velocity,
      std::size_t entity_index) const;
  SpriteDefinition ResolveMissile(
      const std::optional<ecs::VelocityComponent> &velocity) const;
  SpriteDefinition ResolveObstacle(
      const std::optional<ecs::HealthComponent> &health) const;
  SpriteDefinition ResolvePowerup() const;
  engine::render::SpriteDrawParams BuildParams(
      const ecs::PositionComponent &position,
      const ecs::SpriteComponent &sprite,
      const ecs::RenderLayerComponent &layer,
      const std::optional<ecs::VelocityComponent> &velocity,
      std::uint16_t type_code, bool default_face_left,
      const std::shared_ptr<engine::render::Texture2D> &texture) const;
  engine::math::RectF ApplyFlip(const engine::math::RectF &base, bool flip_x,
                                bool flip_y) const;
  engine::math::Vector2f ComputeScale(const engine::render::Texture2D &texture,
                                      const engine::math::RectF &source) const;
  bool ComputeFlipX(std::uint16_t type_code, bool default_left,
                    const std::optional<ecs::VelocityComponent> &velocity,
                    bool sprite_flip) const;
  std::shared_ptr<engine::render::Texture2D> LoadTexture(const std::string &id);

  engine::ecs::Registry &registry_;
  engine::render::Renderer2D &renderer_;
  std::unordered_map<std::string, std::shared_ptr<engine::render::Texture2D>>
      textures_;
  std::vector<DrawCommand> draw_queue_;
  bool debug_hitboxes_{false};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_RENDER_SYSTEM_H_
