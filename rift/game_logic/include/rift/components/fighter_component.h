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

enum class AttackType : std::uint8_t {
  kNone = 0,
  kLight,
  kHeavy
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

  bool IsAlive() const { return current_health > 0; }

  void TakeDamage(std::uint32_t amount) {
    if (invulnerable) return;
    current_health = (current_health > amount) ? current_health - amount : 0;
  }
};

struct StaminaComponent {
  float current_stamina{100.0f};
  float max_stamina{100.0f};
  float regen_rate{10.0f};
  bool regenerating{true};

  bool CanAfford(float cost) const { return current_stamina >= cost; }

  bool Consume(float cost) {
    if (current_stamina < cost) return false;
    current_stamina -= cost;
    return true;
  }

  void Regenerate(float dt) {
    if (!regenerating) return;
    current_stamina = std::min(current_stamina + regen_rate * dt, max_stamina);
  }
};

struct CombatStateComponent {
  CombatState state{CombatState::kIdle};
  std::uint32_t state_timer_ms{0};
  std::uint32_t stun_duration_ms{0};
};

struct AttackComponent {
  AttackType type{AttackType::kNone};
  std::uint32_t startup_frames{0};
  std::uint32_t active_frames{0};
  std::uint32_t recovery_frames{0};
  std::uint32_t current_frame{0};
  std::uint32_t damage{0};
  float stamina_cost{0.0f};
  bool hit_confirmed{false};

  static constexpr std::uint32_t kLightStartup = 4;
  static constexpr std::uint32_t kLightActive = 3;
  static constexpr std::uint32_t kLightRecovery = 6;
  static constexpr std::uint32_t kLightDamage = 10;
  static constexpr float kLightStaminaCost = 10.0f;

  static constexpr std::uint32_t kHeavyStartup = 10;
  static constexpr std::uint32_t kHeavyActive = 5;
  static constexpr std::uint32_t kHeavyRecovery = 15;
  static constexpr std::uint32_t kHeavyDamage = 25;
  static constexpr float kHeavyStaminaCost = 25.0f;

  void StartAttack(AttackType attack_type) {
    type = attack_type;
    current_frame = 0;
    hit_confirmed = false;

    if (attack_type == AttackType::kLight) {
      startup_frames = kLightStartup;
      active_frames = kLightActive;
      recovery_frames = kLightRecovery;
      damage = kLightDamage;
      stamina_cost = kLightStaminaCost;
    } else if (attack_type == AttackType::kHeavy) {
      startup_frames = kHeavyStartup;
      active_frames = kHeavyActive;
      recovery_frames = kHeavyRecovery;
      damage = kHeavyDamage;
      stamina_cost = kHeavyStaminaCost;
    }
  }

  void Reset() {
    type = AttackType::kNone;
    current_frame = 0;
    hit_confirmed = false;
  }

  bool IsInStartup() const {
    return type != AttackType::kNone && current_frame < startup_frames;
  }

  bool IsActive() const {
    return type != AttackType::kNone &&
           current_frame >= startup_frames &&
           current_frame < startup_frames + active_frames;
  }

  bool IsInRecovery() const {
    return type != AttackType::kNone &&
           current_frame >= startup_frames + active_frames &&
           current_frame < startup_frames + active_frames + recovery_frames;
  }

  bool IsComplete() const {
    return type != AttackType::kNone &&
           current_frame >= startup_frames + active_frames + recovery_frames;
  }

  std::uint32_t TotalFrames() const {
    return startup_frames + active_frames + recovery_frames;
  }
};

struct HurtboxComponent {
  float width{30.0f};
  float height{80.0f};
  float offset_x{0.0f};
  float offset_y{0.0f};
};

struct HitboxComponent {
  float width{40.0f};
  float height{20.0f};
  float offset_x{35.0f};
  float offset_y{0.0f};
  bool active{false};
};

struct BlockComponent {
  bool is_blocking{false};
  float damage_reduction{0.75f};
  float stamina_drain_rate{15.0f};

  static constexpr float kGuardBreakRecoveryMs = 1000.0f;
};

struct DodgeComponent {
  bool is_dodging{false};
  float dodge_speed{400.0f};
  float dodge_duration_ms{200.0f};
  float current_time_ms{0.0f};
  float stamina_cost{30.0f};
  float cooldown_ms{500.0f};
  float cooldown_remaining_ms{0.0f};
  std::int8_t direction{0};
};

}  // namespace rift::components

#endif  // RIFT_COMPONENTS_FIGHTER_COMPONENT_H_
