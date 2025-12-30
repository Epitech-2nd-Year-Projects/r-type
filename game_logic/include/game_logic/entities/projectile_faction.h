#ifndef GAME_LOGIC_ENTITIES_PROJECTILE_FACTION_H_
#define GAME_LOGIC_ENTITIES_PROJECTILE_FACTION_H_

#include <cstdint>

namespace game_logic::entities {

/**
 * @enum ProjectileFaction
 * @brief Ownership of projectile for collision filtering
 */
enum class ProjectileFaction : std::uint8_t {
  kPlayer = 0,
  kEnemy = 1,
  kNeutral = 2
};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_PROJECTILE_FACTION_H_
