#ifndef GAME_LOGIC_COMPONENTS_DAMAGEABLE_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_DAMAGEABLE_COMPONENT_H_

#include <cstdint>

namespace game_logic::components {

/**
 * @brief Damage-dealing projectile or hazard
 *
 * @details
 * Attached to projectiles (missiles, bullets). Tracks owner
 * to prevent self-damage. Collision system reads damage value.
 */
struct DamageableComponent {
  /// @brief Entity that spawned this projectile (0 = environment)
  std::uint32_t owner_id{0};

  /// @brief Damage dealt on collision
  std::uint32_t damage{10};

  /// @brief Friendly fire enabled (can damage owner's faction)
  bool friendly_fire{false};

  /// @brief Projectile faction (0=player, 1=enemy, 2=neutral)
  std::uint8_t faction{0};

  DamageableComponent() = default;
  explicit DamageableComponent(std::uint32_t dmg) : damage(dmg) {}
  DamageableComponent(std::uint32_t owner, std::uint32_t dmg)
      : owner_id(owner), damage(dmg) {}
  DamageableComponent(std::uint32_t owner, std::uint32_t dmg, std::uint8_t fac)
      : owner_id(owner), damage(dmg), faction(fac) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_DAMAGEABLE_COMPONENT_H_