#ifndef GAME_LOGIC_COMPONENTS_HEALTH_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_HEALTH_COMPONENT_H_

#include <cstdint>

namespace game_logic::components {

/**
 * @brief Health points for damageable entities
 *
 * @details
 * Tracks current and maximum health. Used by collision systems
 * to apply damage. Entity dies when current_health reaches 0.
 */
struct HealthComponent {
  /// @brief Maximum health capacity
  std::uint32_t max_health{100};

  /// @brief Current health points
  std::uint32_t current_health{100};

  /// @brief Whether entity is invulnerable
  bool invulnerable{false};

  HealthComponent() = default;
  explicit HealthComponent(std::uint32_t hp)
      : max_health(hp), current_health(hp) {}
  HealthComponent(std::uint32_t max_hp, std::uint32_t current_hp)
      : max_health(max_hp), current_health(current_hp) {}

  /**
   * @brief Check if entity is alive
   */
  bool is_alive() const { return current_health > 0; }

  /**
   * @brief Apply damage (respects invulnerability)
   */
  void take_damage(std::uint32_t amount) {
    if (invulnerable) return;
    if (current_health > amount)
      current_health -= amount;
    else
      current_health = 0;
  }

  /**
   * @brief Heal entity (capped at max_health)
   */
  void heal(std::uint32_t amount) {
    current_health = std::min(current_health + amount, max_health);
  }
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_HEALTH_COMPONENT_H_