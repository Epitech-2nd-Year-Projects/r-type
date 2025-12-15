#ifndef GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_

#include <deque>
#include <random>

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/math/vector2.h"
#include "engine/time/time_delta.h"
#include "game_logic/entities/enemy_builder.h"
#include "game_logic/game_config.h"

namespace game_logic {
class GameInstance;
struct LevelConfig;
}  // namespace game_logic

namespace game_logic::systems {

/**
 * @brief Defines a single enemy spawn event
 */
struct WaveEntry {
  float spawn_time{0.0f};
  entities::EnemyType type{entities::EnemyType::kScout};
  engine::math::Vector2f position{0.0f, 0.0f};
  bool random_y{false};
  bool drops_powerup{false};
};

/**
 * @class WaveSystem
 * @brief Manages timed spawning of enemy waves.
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
  float current_wave_time_{0.0f};
  std::deque<WaveEntry> pending_spawns_;
  int current_level_{1};
  bool waiting_for_next_level_{false};
  float level_finished_timer_{0.0f};

  GameInstance &game_instance_;

  // Random Number Generation
  std::mt19937 rng_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
