#ifndef GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_

namespace game_logic {
class GameInstance;
}
namespace engine::scripting {
class PrefabFactory;
}

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"
#include "engine/scripting/script_engine.h"

namespace game_logic::systems {

/**
 * @class HealthSystem
 * @brief Manages entity life cycle, death, and scoring.
 *
 * @details
 * Performs the following:
 * 1. Checks for entities with <= 0 health.
 * 2. Delegates death handling to Lua ("GameEvents.HandleDeath").
 * 3. Cleans up dead entities.
 */
class HealthSystem : public engine::ecs::ISystem {
 public:
  static constexpr float kRespawnBaseX = 100.0f;
  static constexpr float kRespawnSlotOffsetX = 50.0f;
  static constexpr float kRespawnY = 300.0f;

  HealthSystem(GameInstance& game_instance,
               engine::scripting::PrefabFactory& prefab_factory,
               engine::scripting::ScriptEngine& script_engine)
      : game_instance_(game_instance),
        prefab_factory_(prefab_factory),
        script_engine_(script_engine) {}
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
  engine::scripting::PrefabFactory& prefab_factory_;
  engine::scripting::ScriptEngine& script_engine_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_HEALTH_SYSTEM_H_
