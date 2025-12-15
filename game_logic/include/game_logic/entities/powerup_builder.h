#ifndef GAME_LOGIC_ENTITIES_POWERUP_BUILDER_H_
#define GAME_LOGIC_ENTITIES_POWERUP_BUILDER_H_

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"
#include "game_logic/game_config.h"

namespace game_logic::entities {

/**
 * @class PowerupBuilder
 * @brief Factory for creating powerup entities
 *
 * @details
 * Creates collectible powerup items based on configuration.
 */
class PowerupBuilder {
 public:
  /**
   * @brief Create a powerup entity
   * @param registry ECS registry
   * @param position Spawn position
   * @param config Configuration for the powerup to spawn
   * @return EntityId of created powerup
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry &registry,
                                      const engine::math::Vector2f &position,
                                      const PowerupConfig &config);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_POWERUP_BUILDER_H_
