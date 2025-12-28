#ifndef GAME_LOGIC_GAME_CONFIG_H_
#define GAME_LOGIC_GAME_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/render/color.h"
#include "game_logic/components/ai_component.h"
#include "game_logic/components/powerup_component.h"

namespace game_logic {

/**
 * @brief World configuration parameters
 */
struct WorldConfig {
  float grid_cell_size;
  float player_spawn_base_x;
  float player_spawn_offset_x;
  float player_spawn_y;
  float spawn_min_y;
  float spawn_max_y;
  float patrol_min_x;
  float patrol_min_y;
  float patrol_max_x;
  float patrol_max_y;
};

/**
 * @struct EnemyConfig
 * @brief Data structure for enemy archetype configuration
 */
struct EnemyConfig {
  std::string name;
  std::uint32_t health;
  float speed;
  std::string behavior_type;
  std::uint32_t score;
  float sprite_width;
  float sprite_height;
  float hitbox_width;
  float hitbox_height;
  std::string texture_path;
  float wave_amplitude{0.0f};
  float wave_frequency{0.0f};
  float detection_range{0.0f};
  bool can_shoot{false};
  float fire_rate{0.0f};
};

/**
 * @struct WaveSpawnConfig
 * @brief Definition of a single spawn in a wave
 */
struct WaveSpawnConfig {
  float time;                 ///< Time in seconds when this spawn occurs
  std::string enemy_type;     ///< Name of the enemy archetype to spawn
  float x;                    ///< X coordinate for spawn
  float y;                    ///< Y coordinate for spawn
  bool random_y;              ///< If true, Y is randomized within spawn bounds
  bool drops_powerup{false};  ///< If true, this enemy drops a powerup on death
};

/**
 * @struct LevelConfig
 * @brief Full level configuration
 */
struct LevelConfig {
  int id;
  std::vector<WaveSpawnConfig> waves;
};

/**
 * @class GameConfig
 * @brief Singleton manager for loading and accessing game configuration.
 *
 * Loads all configuration from JSON files in the config/ directory.
 * Provides read-only access to the data.
 */
class GameConfig {
 public:
  static GameConfig &Get();

  /**
   * @brief Load all configuration files.
   * @param config_dir Path to the configuration directory (e.g. "config")
   * @return true if successful, false otherwise
   */
  bool Load(const std::string &config_dir);

  const WorldConfig &GetWorld() const { return world_config_; }

  /**
   * @brief Get enemy configuration by name
   * @param name Enemy archetype name
   * @return Constant reference to EnemyConfig
   * @throw std::runtime_error if not found
   */
  const EnemyConfig &GetEnemy(const std::string &name) const;

  /**
   * @brief Get level configuration by ID
   * @param id Level ID
   * @return Constant reference to LevelConfig
   * @throw std::runtime_error if not found
   */
  const LevelConfig &GetLevel(int id) const;

  const std::string &GetConfigDirectory() const { return config_dir_; }

 private:
  GameConfig() = default;
  ~GameConfig() = default;

  GameConfig(const GameConfig &) = delete;
  GameConfig &operator=(const GameConfig &) = delete;

  std::string config_dir_;
  WorldConfig world_config_;
  std::unordered_map<std::string, EnemyConfig> enemies_;
  std::unordered_map<int, LevelConfig> levels_;
};

}  // namespace game_logic

#endif  // GAME_LOGIC_GAME_CONFIG_H_
