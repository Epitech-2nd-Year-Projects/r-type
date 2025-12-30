#ifndef GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_

#include <deque>
#include <random>
#include <string>

#include "engine/ecs/registry.h"
#include "game_logic/game_config.h"

namespace game_logic {
class GameInstance;
struct LevelConfig;
}  // namespace game_logic

namespace game_logic::systems {

/**
 * @brief Defines a single enemy spawn event
 */
class WaveSystem : public engine::ecs::ISystem {
 public:
  explicit WaveSystem(GameInstance &game_instance);

  ~WaveSystem() override = default;

  /**
   * @brief Update wave timer and spawn enemies.
   */
  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;

  /**
   * @brief Load a specific wave configuration (for now hardcoded)
   */
  void LoadLevel(int level_id);

 private:
  GameInstance &game_instance_;
  std::mt19937 rng_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
