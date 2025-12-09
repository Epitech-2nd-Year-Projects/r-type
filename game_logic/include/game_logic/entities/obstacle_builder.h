#ifndef GAME_LOGIC_ENTITIES_OBSTACLE_BUILDER_H_
#define GAME_LOGIC_ENTITIES_OBSTACLE_BUILDER_H_

#include <cstdint>
#include <optional>
#include <string>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"

namespace game_logic::entities {

/**
 * @enum ObstacleType
 * @brief Types of level obstacles
 */
enum class ObstacleType : std::uint8_t {
  kIndestructible = 0,
  kDestructible = 1
};

/**
 * @struct ObstacleConfig
 * @brief Obstacle entity creation configuration
 */
struct ObstacleConfig {
  ObstacleType type{ObstacleType::kIndestructible};
  engine::math::Vector2f position{0.0f, 0.0f};
  float width{64.0f};
  float height{64.0f};
  std::uint32_t health{100};
  std::uint32_t score_value{50};
  std::optional<std::string> custom_texture{};
};

/**
 * @class ObstacleBuilder
 * @brief Factory for creating level obstacle entities
 *
 * @details
 * ObstacleBuilder creates static or destructible obstacles with:
 * - Position and collision components
 * - Optional health for destructible obstacles
 * - Rendering components
 *
 * Two types of obstacles are supported:
 * - **Indestructible**: Solid barriers, no health
 * - **Destructible**: Can be destroyed, has health and score
 */
class ObstacleBuilder {
 public:
  /**
   * @brief Create obstacle with full configuration
   * @param registry ECS registry
   * @param config Obstacle configuration
   * @return EntityId of created obstacle
   *
   * @details
   * Attaches the following components:
   * - PositionComponent (position)
   * - BoundingBoxComponent (collision, full size)
   * - SpriteComponent (visuals)
   * - TagComponent ("Obstacle")
   *
   * Additionally for destructible obstacles:
   * - HealthComponent (HP)
   * - ScoreValueComponent (points)
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry& registry,
                                      const ObstacleConfig& config);

  /**
   * @brief Create indestructible wall
   * @param registry ECS registry
   * @param position Top-left corner position
   * @param width Width in pixels
   * @param height Height in pixels
   * @return EntityId of created wall
   */
  static engine::ecs::EntityId CreateWall(
      engine::ecs::Registry& registry, const engine::math::Vector2f& position,
      float width, float height);

  /**
   * @brief Create destructible barrier
   * @param registry ECS registry
   * @param position Top-left corner position
   * @param width Width in pixels
   * @param height Height in pixels
   * @param health Health points
   * @return EntityId of created barrier
   */
  static engine::ecs::EntityId CreateDestructibleBarrier(
      engine::ecs::Registry& registry, const engine::math::Vector2f& position,
      float width, float height, std::uint32_t health);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_OBSTACLE_BUILDER_H_
