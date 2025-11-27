#ifndef GAME_LOGIC_COMPONENTS_POWERUP_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_POWERUP_COMPONENT_H_

#include <cstdint>

#include "engine/time/time_delta.h"

namespace game_logic::components {

/**
 * @enum PowerupType
 * @brief Available power-up types
 */
enum class PowerupType : std::uint8_t {
  kHealth = 0,         ///< Restore health
  kWeaponUpgrade = 1,  ///< Improve weapon
  kSpeedBoost = 2,     ///< Temporary speed increase
  kShield = 3,         ///< Temporary invulnerability
  kExtraLife = 4,      ///< Grant additional life
  kScore = 5           ///< Bonus points
};

/**
 * @brief Collectible power-up item
 *
 * @details
 * Spawned by certain enemies on death. Collision with player
 * triggers effect and destroys power-up entity.
 */
struct PowerupComponent {
  /// @brief Power-up type
  PowerupType type{PowerupType::kHealth};

  /// @brief Time before despawn (0 = permanent)
  engine::time::TimeDelta lifetime{
      engine::time::TimeDelta::from_seconds(10.0f)};

  /// @brief Power-up is active (not yet collected)
  bool active{true};

  /// @brief Numeric value (health restored, score bonus, etc.)
  std::uint32_t value{50};

  PowerupComponent() = default;
  explicit PowerupComponent(PowerupType t) : type(t) {}
  PowerupComponent(PowerupType t, std::uint32_t val) : type(t), value(val) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_POWERUP_COMPONENT_H_