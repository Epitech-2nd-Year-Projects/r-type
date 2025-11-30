#ifndef GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_
#define GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_

#include <cstdint>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"

namespace game_logic::entities {

/**
 * @enum ProjectileFaction
 * @brief Projectile ownership faction
 */
enum class ProjectileFaction : std::uint8_t {
  kPlayer = 0,
  kEnemy = 1,
  kNeutral = 2
};

/**
 * @struct MissileConfig
 * @brief Missile entity creation configuration
 */
struct MissileConfig {
  engine::math::Vector2f spawn_position{0.0f, 0.0f};
  engine::math::Vector2f velocity{200.0f, 0.0f};
  std::uint32_t damage{10};
  float lifetime{5.0f};
  std::uint32_t owner_id{0};
  ProjectileFaction faction{ProjectileFaction::kPlayer};
  bool friendly_fire{false};
  float sprite_width{16.0f};
  float sprite_height{8.0f};
};

/**
 * @class MissileBuilder
 * @brief Factory for creating projectile entities
 *
 * @details
 * MissileBuilder creates missile/bullet entities with:
 * - Position and velocity components
 * - Damage and owner tracking
 * - Automatic lifetime expiration
 * - Collision detection
 * - Faction-based targeting
 */
class MissileBuilder {
 public:
  /**
   * @brief Create missile with full configuration
   * @param registry ECS registry
   * @param config Missile configuration
   * @return EntityId of created missile
   *
   * @details
   * Attaches the following components:
   * - PositionComponent (spawn position)
   * - VelocityComponent (initial velocity)
   * - DamageableComponent (damage, owner, faction)
   * - LifetimeComponent (auto-destruction)
   * - BoundingBoxComponent (collision)
   * - SpriteComponent (visuals)
   * - TagComponent ("Missile")
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry& registry,
                                      const MissileConfig& config);

  /**
   * @brief Create player projectile
   * @param registry ECS registry
   * @param owner_id Player entity ID
   * @param spawn_position Initial position
   * @param velocity Initial velocity
   * @return EntityId of created missile
   */
  static engine::ecs::EntityId CreatePlayerMissile(
      engine::ecs::Registry& registry, std::uint32_t owner_id,
      const engine::math::Vector2f& spawn_position,
      const engine::math::Vector2f& velocity);

  /**
   * @brief Create enemy projectile
   * @param registry ECS registry
   * @param owner_id Enemy entity ID
   * @param spawn_position Initial position
   * @param velocity Initial velocity
   * @return EntityId of created missile
   */
  static engine::ecs::EntityId CreateEnemyMissile(
      engine::ecs::Registry& registry, std::uint32_t owner_id,
      const engine::math::Vector2f& spawn_position,
      const engine::math::Vector2f& velocity);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_