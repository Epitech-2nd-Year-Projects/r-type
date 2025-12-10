#ifndef GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_
#define GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_

#include <cstdint>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"
#include "game_logic/entities/missile_config.h"
#include "game_logic/entities/missile_data.h"

namespace game_logic::entities {

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
  static engine::ecs::EntityId Create(engine::ecs::Registry &registry,
                                      const MissileConfig &config,
                                      const MissileArchetypeData &archetype);

  /**
   * @brief Create a missile from an archetype
   * @param registry ECS registry
   * @param owner_id Entity ID of the shooter
   * @param spawn_position Spawn position
   * @param velocity Missile velocity
   * @param data Archetype data (stats, sprite, etc.)
   * @param faction Projectile faction
   * @return EntityId of created missile
   */
  static engine::ecs::EntityId CreateMissile(
      engine::ecs::Registry &registry, std::uint32_t owner_id,
      const engine::math::Vector2f &spawn_position,
      const engine::math::Vector2f &velocity, const MissileArchetypeData &data,
      ProjectileFaction faction);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_MISSILE_BUILDER_H_