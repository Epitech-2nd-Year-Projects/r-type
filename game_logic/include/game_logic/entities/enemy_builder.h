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
  kScout = 0,   ///< Fast, weak, straight movement
  kBomber = 1,  ///< Medium speed, wave pattern
  kTank = 2     ///< Slow, high HP, patrol pattern
};

/**
 * @struct EnemyConfig
 * @brief Enemy entity creation configuration
 */
struct EnemyConfig {
  /// @brief Enemy archetype
  EnemyType type{EnemyType::kScout};

  /// @brief Spawn position
  engine::math::Vector2f spawn_position{800.0f, 300.0f};

  /// @brief Initial velocity (optional, uses default if zero)
  engine::math::Vector2f initial_velocity{0.0f, 0.0f};

  /// @brief Custom health (0 = use type default)
  std::uint32_t custom_health{0};

  /// @brief Custom score value (0 = use type default)
  std::uint32_t custom_score{0};

  /// @brief Custom AI speed (0.0f = use type default)
  float custom_speed{0.0f};

  /// @brief Custom AI behavior (optional override)
  components::EnemyBehavior custom_behavior{
      components::EnemyBehavior::kStraight};
  bool use_custom_behavior{false};
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
 *
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
   * - TagComponent ("Enemy")
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry& registry,
                                      const EnemyConfig& config);

  /**
   * @brief Create enemy with minimal parameters
   * @param registry ECS registry
   * @param type Enemy archetype
   * @param spawn_position Initial position
   * @return EntityId of created enemy
   */
  static engine::ecs::EntityId Create(
      engine::ecs::Registry& registry, EnemyType type,
      const engine::math::Vector2f& spawn_position);

  /**
   * @brief Create Scout enemy (fast, weak)
   * @param registry ECS registry
   * @param spawn_position Initial position
   * @return EntityId of created scout
   */
  static engine::ecs::EntityId CreateScout(
      engine::ecs::Registry& registry,
      const engine::math::Vector2f& spawn_position);

  /**
   * @brief Create Bomber enemy (medium, wave pattern)
   * @param registry ECS registry
   * @param spawn_position Initial position
   * @return EntityId of created bomber
   */
  static engine::ecs::EntityId CreateBomber(
      engine::ecs::Registry& registry,
      const engine::math::Vector2f& spawn_position);

  /**
   * @brief Create Tank enemy (slow, tough)
   * @param registry ECS registry
   * @param spawn_position Initial position
   * @return EntityId of created tank
   */
  static engine::ecs::EntityId CreateTank(
      engine::ecs::Registry& registry,
      const engine::math::Vector2f& spawn_position);

 private:
  /// @brief Scout sprite dimensions (pixels)
  static constexpr float kScoutWidth = 24.0f;
  static constexpr float kScoutHeight = 24.0f;
  static constexpr float kScoutHitboxWidth = 20.0f;
  static constexpr float kScoutHitboxHeight = 20.0f;

  /// @brief Bomber sprite dimensions (pixels)
  static constexpr float kBomberWidth = 32.0f;
  static constexpr float kBomberHeight = 32.0f;
  static constexpr float kBomberHitboxWidth = 28.0f;
  static constexpr float kBomberHitboxHeight = 28.0f;

  /// @brief Tank sprite dimensions (pixels)
  static constexpr float kTankWidth = 48.0f;
  static constexpr float kTankHeight = 48.0f;
  static constexpr float kTankHitboxWidth = 44.0f;
  static constexpr float kTankHitboxHeight = 44.0f;

  /// @brief Scout stats
  static constexpr std::uint32_t kScoutHealth = 30;
  static constexpr float kScoutSpeed = 150.0f;
  static constexpr std::uint32_t kScoutScore = 100;

  /// @brief Bomber stats
  static constexpr std::uint32_t kBomberHealth = 60;
  static constexpr float kBomberSpeed = 100.0f;
  static constexpr std::uint32_t kBomberScore = 200;

  /// @brief Tank stats
  static constexpr std::uint32_t kTankHealth = 150;
  static constexpr float kTankSpeed = 50.0f;
  static constexpr std::uint32_t kTankScore = 500;

  /// @brief Texture paths
  static constexpr const char* kScoutTexturePath =
      "assets/sprites/enemy_scout.png";
  static constexpr const char* kBomberTexturePath =
      "assets/sprites/enemy_bomber.png";
  static constexpr const char* kTankTexturePath =
      "assets/sprites/enemy_tank.png";

  /**
   * @brief Get default velocity for enemy type
   * @param type Enemy archetype
   * @return Initial velocity vector
   */
  static engine::math::Vector2f GetDefaultVelocity(EnemyType type);

  /**
   * @brief Get sprite dimensions for enemy type
   * @param type Enemy archetype
   * @return {width, height} in pixels
   */
  static engine::math::Vector2f GetSpriteDimensions(EnemyType type);

  /**
   * @brief Get hitbox dimensions for enemy type
   * @param type Enemy archetype
   * @return {width, height} in pixels
   */
  static engine::math::Vector2f GetHitboxDimensions(EnemyType type);

  /**
   * @brief Get texture path for enemy type
   * @param type Enemy archetype
   * @return Texture asset path
   */
  static const char* GetTexturePath(EnemyType type);

  /**
   * @brief Get default health for enemy type
   * @param type Enemy archetype
   * @return Health points
   */
  static std::uint32_t GetDefaultHealth(EnemyType type);

  /**
   * @brief Get default AI speed for enemy type
   * @param type Enemy archetype
   * @return Speed in pixels/second
   */
  static float GetDefaultSpeed(EnemyType type);

  /**
   * @brief Get default AI behavior for enemy type
   * @param type Enemy archetype
   * @return Behavior pattern
   */
  static components::EnemyBehavior GetDefaultBehavior(EnemyType type);

  /**
   * @brief Get default score value for enemy type
   * @param type Enemy archetype
   * @return Points awarded on death
   */
  static std::uint32_t GetDefaultScore(EnemyType type);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_ENEMY_BUILDER_H_