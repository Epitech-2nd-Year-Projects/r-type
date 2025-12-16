#ifndef GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_

namespace game_logic {
class GameInstance;
}

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class HealthSystem
 * @brief Manages entity life cycle, death, and scoring.
 *
 * @details
 * Performs the following:
 * 1. Checks for entities with <= 0 health.
 * 2. Handles Player death (lives--, respawn, invulnerability).
 * 3. Handles Enemy death (awards score to attacker, destroys entity).
 * 4. Cleans up dead entities.
 */
class HealthSystem : public engine::ecs::ISystem {
 public:
  static constexpr float kRespawnBaseX = 100.0f;
  static constexpr float kRespawnSlotOffsetX = 50.0f;
  static constexpr float kRespawnY = 300.0f;

  explicit HealthSystem(GameInstance& game_instance)
      : game_instance_(game_instance) {}
  ~HealthSystem() override = default;

  /**
   * @brief Update health and life cycle logic.
   *
   * @param registry Reference to ECS registry.
   * @param dt Time since last frame.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  GameInstance& game_instance_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_
