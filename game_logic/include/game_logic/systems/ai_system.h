#ifndef GAME_LOGIC_SYSTEMS_AI_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_AI_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class AISystem
 * @brief Manages enemy behavior and movement patterns.
 *
 * @details
 * Updates VelocityComponent based on AIComponent configuration.
 * Supported behaviors:
 * - Straight: Moves in a fixed direction.
 * - Wave: Moves in a sine wave pattern.
 * - ChasePlayer: Adjusts velocity to target the nearest player.
 */
class AISystem : public engine::ecs::ISystem {
 public:
  explicit AISystem(engine::scripting::ScriptEngine& script_engine);
  ~AISystem() override = default;

  /**
   * @brief Update AI entities.
   *
   * @param registry Reference to ECS registry.
   * @param dt Time since last frame.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  engine::scripting::ScriptEngine& script_engine_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_AI_SYSTEM_H_
