#ifndef GAME_LOGIC_SYSTEMS_GAME_STATE_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_GAME_STATE_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic {
class GameInstance;  // Forward declaration
}

namespace game_logic::systems {

/**
 * @class GameStateSystem
 * @brief Manages global game state, win/loss conditions, and scoring.
 */
class GameStateSystem : public engine::ecs::ISystem {
 public:
  explicit GameStateSystem(GameInstance &game_instance);
  ~GameStateSystem() override = default;

  /**
   * @brief Update game state based on component data.
   * @param registry ECS registry
   * @param dt Time delta
   */
  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;

 private:
  GameInstance &game_instance_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_GAME_STATE_SYSTEM_H_
