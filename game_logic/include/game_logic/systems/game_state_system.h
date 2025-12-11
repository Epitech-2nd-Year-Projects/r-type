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
 *
 * @details
 * The GameStateSystem is responsible for synchronizing ECS component data
 * with the high-level GameState snapshot used for networking and game flow.
 *
 * Key responsibilities:
 * - **Score Synchronization**: Iterates over all entities with a
 * PlayerComponent and updates the corresponding entry in
 * GameState::player_scores.
 * - **Game Over Condition**: Checks if all active players have 0 lives. If so,
 *   sets GameState::is_game_over to true.
 * - **Level Progression**: (Future) Monitors wave completion to advance levels.
 *
 * This system runs at the end of the frame (or fixed tick) to ensure the
 * GameState is up-to-date before it is snapshot by the server for broadcasting.
 */
class GameStateSystem : public engine::ecs::ISystem {
 public:
  explicit GameStateSystem(GameInstance &game_instance);
  ~GameStateSystem() override = default;

  /**
   * @brief Update game state based on component data.
   * @param registry ECS registry containing game entities.
   * @param dt Time delta since last update.
   *
   * @details
   * Performs the following updates:
   * 1. **Player State Sync**: Reads `PlayerComponent` data (score, lives) from
   *    all player entities and updates the matching records in
   *    `GameInstance::State()::player_scores`.
   * 2. **Game Over Check**: Updates `is_alive` status for each player. If all
   *    players are dead (lives == 0), sets `is_game_over = true` and
   *    `is_running = false`.
   *
   * This ensures the authoritative GameState reflects the latest result of
   * all other systems (Collision, Health, etc.).
   */
  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;

 private:
  GameInstance &game_instance_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_GAME_STATE_SYSTEM_H_
