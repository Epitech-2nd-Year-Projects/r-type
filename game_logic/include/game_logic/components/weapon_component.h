#ifndef GAME_LOGIC_COMPONENTS_WEAPON_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_WEAPON_COMPONENT_H_

#include <cstdint>

#include "engine/time/time_delta.h"

namespace game_logic::components {

/**
 * @enum WeaponType
 * @brief Available weapon types
 */
enum class WeaponType : std::uint8_t {
  kBasic = 0,    ///< Standard single shot
  kDouble = 1,   ///< Double parallel shots
  kSpread = 2,   ///< Spread shot pattern
  kLaser = 3,    ///< Continuous laser beam
  kMissile = 4,  ///< Homing missiles
  kWave = 5      ///< Wave beam
};

/**
 * @brief Weapon firing system
 *
 * @details
 * Manages weapon cooldown, fire rate, ammo (limited or unlimited),
 * and weapon type. WeaponSystem uses this to spawn projectiles.
 */
struct WeaponComponent {
  /// @brief Weapon type
  WeaponType type{WeaponType::kBasic};

  /// @brief Shots per second
  float fire_rate{2.0f};

  /// @brief Remaining cooldown before next shot
  engine::time::TimeDelta cooldown_remaining{engine::time::TimeDelta::zero()};

  /// @brief Whether weapon has unlimited ammo
  bool has_unlimited_ammo{true};

  /// @brief Current ammo count (only used if !has_unlimited_ammo)
  std::uint32_t ammo_count{0};

  /// @brief Weapon power level (1-5)
  std::uint8_t power_level{1};

  /// @brief Whether the trigger is currently held by the player
  bool is_trigger_held{false};

  WeaponComponent() = default;
  explicit WeaponComponent(WeaponType t) : type(t) {}
  WeaponComponent(WeaponType t, float rate) : type(t), fire_rate(rate) {}

  /**
   * @brief Check if weapon can fire
   * @return true if cooldown expired and ammo available
   */
  bool can_fire() const {
    if (cooldown_remaining > engine::time::TimeDelta::zero()) return false;
    if (has_unlimited_ammo) return true;
    return ammo_count > 0;
  }

  /**
   * @brief Trigger weapon fire (resets cooldown, consumes ammo if limited)
   * @param rate Fire rate (shots per second)
   */
  void fire(float rate) {
    if (rate <= 0.0f) return;
    cooldown_remaining = engine::time::TimeDelta::from_seconds(1.0f / rate);
    if (!has_unlimited_ammo && ammo_count > 0) {
      ammo_count--;
    }
  }

  /**
   * @brief Add ammo
   * @param amount Amount to add
   */
  void add_ammo(std::uint32_t amount) {
    if (!has_unlimited_ammo) {
      ammo_count += amount;
    }
  }

  /**
   * @brief Set unlimited ammo
   */
  void set_unlimited_ammo() {
    has_unlimited_ammo = true;
    ammo_count = 0;
  }

  /**
   * @brief Set limited ammo
   */
  void set_limited_ammo(std::uint32_t count) {
    has_unlimited_ammo = false;
    has_unlimited_ammo = false;
    ammo_count = count;
  }

  /// @brief Whether the big shot trigger is currently held
  bool is_big_trigger_held{false};

  /// @brief Remaining cooldown before next big shot
  engine::time::TimeDelta big_shot_cooldown_remaining{
      engine::time::TimeDelta::zero()};

  /**
   * @brief Check if weapon can fire big shot
   * @return true if cooldown expired
   */
  bool can_fire_big() const {
    return big_shot_cooldown_remaining <= engine::time::TimeDelta::zero();
  }

  /**
   * @brief Trigger big weapon fire (resets cooldown)
   * @param rate Fire rate (shots per second)
   */
  void fire_big(float rate) {
    if (rate <= 0.0f) return;
    big_shot_cooldown_remaining =
        engine::time::TimeDelta::from_seconds(1.0f / rate);
  }
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_WEAPON_COMPONENT_H_