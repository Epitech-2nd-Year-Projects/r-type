#ifndef GAME_LOGIC_ENTITIES_ENEMY_BUILDER_H_
#define GAME_LOGIC_ENTITIES_ENEMY_BUILDER_H_

#include <cstdint>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"
#include "game_logic/components/ai_component.h"

namespace game_logic::entities {

/**
 * @enum EnemyType
 * @brief Available enemy archetypes
 */
enum class EnemyType : std::uint8_t {
  kScout = 0,
  kBomber = 1,
  kTank = 2,
  kInterceptor = 3
};

/**
 * @struct EnemyConfig
 * @brief Enemy entity creation configuration
 */
struct EnemySpawnConfig {
  EnemyType type{EnemyType::kScout};
  engine::math::Vector2f spawn_position{0.0f, 0.0f};
  engine::math::Vector2f initial_velocity{0.0f, 0.0f};

  std::uint32_t custom_health{0};
  std::uint32_t custom_score{0};

  bool use_custom_behavior{false};
  components::EnemyBehavior custom_behavior{
      components::EnemyBehavior::kStraight};
  float custom_speed{0.0f};
};

/**
 * @class EnemyBuilder
 * @brief Factory for creating enemy entities
 *
 * @details
 * EnemyBuilder creates fully-configured Bydo enemy entities with:
 * - Position and velocity components
 * - Health, AI, and score systems
 * - Collision and rendering components
 */
class EnemyBuilder {
 public:
  /**
   * @brief Create enemy entity with full configuration
   * @param registry ECS registry
   * @param config Enemy configuration
   * @return EntityId of created enemy
   *
   * @details
   * Attaches the following components:
   * - PositionComponent (spawn position)
   * - VelocityComponent (type-specific or custom)
   * - HealthComponent (type-specific or custom HP)
   * - AIComponent (behavior, speed, detection)
   * - ScoreValueComponent (points awarded on death)
   * - BoundingBoxComponent (collision)
   * - SpriteComponent (visuals)
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry &registry,
                                      const EnemySpawnConfig &config);

  /**
   * @brief Create enemy with minimal parameters
   * @param registry ECS registry
   * @param type Enemy archetype
   * @param spawn_position Initial position
   * @return EntityId of created enemy
   */
  static engine::ecs::EntityId Create(
      engine::ecs::Registry &registry, EnemyType type,
      const engine::math::Vector2f &spawn_position);

  /**
   * @brief Create a PataPata enemy (Basic Scout)
   * @param registry ECS registry
   * @param spawn_position Initial position
   * @return EntityId of created enemy
   *
   * @details Creates an enemy mapped to the 'Scout' archetype.
   */
  static engine::ecs::EntityId CreatePataPata(
      engine::ecs::Registry &registry,
      const engine::math::Vector2f &spawn_position);

  /**
   * @brief Create a Bydo enemy (Bomber)
   * @param registry ECS registry
   * @param spawn_position Initial position
   * @return EntityId of created enemy
   *
   * @details Creates an enemy mapped to the 'Bomber' archetype.
   */
  static engine::ecs::EntityId CreateBydo(
      engine::ecs::Registry &registry,
      const engine::math::Vector2f &spawn_position);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_ENEMY_BUILDER_H_