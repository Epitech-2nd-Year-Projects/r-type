#ifndef GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_

#include <deque>

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/math/vector2.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @brief Types of enemies that can be spawned in a wave
 * @details Mapped to specific EnemyBuilder archetypes
 */
enum class WaveEnemyType {
  kPataPata,
  kBydo
  // more is comming
};

/**
 * @brief Defines a single enemy spawn event
 */
struct WaveEntry {
  float spawn_time{0.0f};  ///< Time in seconds since wave start
  WaveEnemyType type{WaveEnemyType::kPataPata};
  engine::math::Vector2f position{0.0f, 0.0f};

  // Optional: override default AI behavior?
  // For now, rely on EnemyBuilder defaults or separate config
};

/**
 * @class WaveSystem
 * @brief Manages timed spawning of enemy waves.
 */
class WaveSystem : public engine::ecs::ISystem {
 public:
  WaveSystem();
  ~WaveSystem() override = default;

  /**
   * @brief Update wave timer and spawn enemies.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

  /**
   * @brief Load a specific wave configuration (for now hardcoded)
   */
  void LoadLevel(int level_id);

 private:
  float current_wave_time_{0.0f};
  std::deque<WaveEntry> pending_spawns_;
  int current_level_{1};
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_WAVE_SYSTEM_H_
