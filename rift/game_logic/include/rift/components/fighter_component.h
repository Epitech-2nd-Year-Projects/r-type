#ifndef RIFT_COMPONENTS_FIGHTER_COMPONENT_H_
#define RIFT_COMPONENTS_FIGHTER_COMPONENT_H_

#include <cstdint>

namespace rift::components {

enum class CombatState : std::uint8_t {
  kIdle = 0,
  kWalking,
  kAttacking,
  kBlocking,
  kStunned,
  kDodging
};

struct FighterComponent {
  std::uint32_t player_id{0};
  std::uint8_t slot{0};  // 0 = P1 (left), 1 = P2 (right)
  std::uint8_t rounds_won{0};
  bool facing_right{true};
};

struct HealthComponent {
  std::uint32_t current_health{100};
  std::uint32_t max_health{100};
  bool invulnerable{false};
};

struct StaminaComponent {
  float current_stamina{100.0f};
  float max_stamina{100.0f};
  float regen_rate{10.0f};
};

struct CombatStateComponent {
  CombatState state{CombatState::kIdle};
  std::uint32_t state_timer_ms{0};
};

}  // namespace rift::components

#endif  // RIFT_COMPONENTS_FIGHTER_COMPONENT_H_
