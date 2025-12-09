#ifndef GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_
#define GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_

#include <cstdint>
#include <string_view>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"

namespace game_logic::entities {

/**
 * @struct PlayerConfig
 * @brief Player entity creation configuration
 */
struct PlayerConfig {
  /// @brief Player unique identifier
  std::uint32_t player_id{0};

  /// @brief Game room identifier
  std::uint32_t room_id{0};

  /// @brief Player slot (0-3)
  std::uint8_t player_slot{0};

  /// @brief Spawn position
  engine::math::Vector2f spawn_position{100.0f, 300.0f};

  /// @brief Initial health
  std::uint32_t initial_health{100};

  /// @brief Initial lives
  std::uint32_t initial_lives{3};
};

/**
 * @class PlayerBuilder
 * @brief Factory for creating player entities
 *
 * @details
 * PlayerBuilder creates fully-configured player entities with:
 * - Position and velocity components
 * - Player metadata (ID, slot, score, lives)
 * - Health and weapon systems
 * - Collision and rendering components
 *
 */
class PlayerBuilder {
 public:
  /**
   * @brief Create player entity with configuration
   * @param registry ECS registry
   * @param config Player configuration
   * @return EntityId of created player
   *
   * @details
   * Attaches the following components:
   * - PositionComponent (spawn position)
   * - VelocityComponent (zero velocity)
   * - PlayerComponent (ID, slot, score, lives)
   * - HealthComponent (initial HP)
   * - WeaponComponent (basic weapon)
   * - BoundingBoxComponent (collision)
   * - SpriteComponent (visuals)
   * - TagComponent ("Player")
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry& registry,
                                      const PlayerConfig& config);

  /**
   * @brief Create player with minimal parameters
   * @param registry ECS registry
   * @param player_id Unique player ID
   * @param room_id Game room ID
   * @param player_slot Player slot (0-3)
   * @param spawn_position Initial position
   * @return EntityId of created player
   *
   * @details
   * Convenience overload using default configuration values.
   */
  static engine::ecs::EntityId Create(
      engine::ecs::Registry& registry, std::uint32_t player_id,
      std::uint32_t room_id, std::uint8_t player_slot,
      const engine::math::Vector2f& spawn_position);

 private:
  /// @brief Default player sprite size (pixels)
  static constexpr float kPlayerWidth = 32.0f;
  static constexpr float kPlayerHeight = 32.0f;

  /// @brief Player hitbox size (smaller than sprite for gameplay)
  static constexpr float kHitboxWidth = 24.0f;
  static constexpr float kHitboxHeight = 24.0f;

  /// @brief Default player movement speed (pixels/second)
  static constexpr float kDefaultMoveSpeed = 200.0f;

  /// @brief Default player sprite texture path
  static constexpr std::string_view kPlayerTexturePath =
      "assets/sprites/player.png";
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_
