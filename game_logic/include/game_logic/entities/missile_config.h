#ifndef GAME_LOGIC_ENTITIES_MISSILE_CONFIG_H_
#define GAME_LOGIC_ENTITIES_MISSILE_CONFIG_H_

#include <cstdint>

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

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_MISSILE_CONFIG_H_
