#ifndef GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_
#define GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_

#include <cstdint>
#include <string_view>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"

namespace game_logic::entities {

/**
 * @struct PlayerSpawnContext
 * @brief Player entity creation context
 */
struct PlayerSpawnContext {
  /// @brief Player unique identifier
  std::uint32_t player_id{0};

  /// @brief Game room identifier
  std::uint32_t room_id{0};

  /// @brief Player slot (0-3)
  std::uint8_t player_slot{0};

  /// @brief Spawn position
  engine::math::Vector2f spawn_position{100.0f, 300.0f};

  /// @brief Initial health override (0 = use config)
  std::uint32_t initial_health{0};

  /// @brief Initial lives override (0 = use config)
  std::uint32_t initial_lives{0};
};

/**
 * @class PlayerBuilder
 * @brief Factory for creating player entities
 */
class PlayerBuilder {
 public:
  /**
   * @brief Create player entity with configuration
   * @param registry ECS registry
   * @param ctx Player creation context with ID, slot, and overrides
   * @return EntityId of created player
   */
  static engine::ecs::EntityId Create(engine::ecs::Registry& registry,
                                      const PlayerSpawnContext& ctx);

  /**
   * @brief Create player with minimal parameters
   * @param registry ECS registry
   * @param player_id Unique player ID
   * @param room_id Game room ID
   * @param player_slot Player slot index (0-3)
   * @param spawn_position Initial spawn position
   * @return EntityId of created player
   */
  static engine::ecs::EntityId Create(
      engine::ecs::Registry& registry, std::uint32_t player_id,
      std::uint32_t room_id, std::uint8_t player_slot,
      const engine::math::Vector2f& spawn_position);
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_PLAYER_BUILDER_H_
