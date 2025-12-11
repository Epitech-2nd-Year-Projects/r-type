#ifndef GAME_LOGIC_GAME_CONFIG_H_
#define GAME_LOGIC_GAME_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/render/color.h"
#include "game_logic/components/ai_component.h"

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
 * @brief Default player configuration
 */
struct PlayerConfig {
  std::uint32_t health;
  std::uint32_t lives;
  float speed;
  float hitbox_width;
  float hitbox_height;
  float sprite_width;
  float sprite_height;
  std::string texture_path;
};

/**
 * @struct EnemyConfig
 * @brief Data structure for enemy archetype configuration
 */
struct EnemyConfig {
  std::string name;
  std::uint32_t health;
  float speed;
  std::string behavior_type;  // String representation of behavior
  std::uint32_t score;
  float sprite_width;
  float sprite_height;
  float hitbox_width;
  float hitbox_height;
  std::string texture_path;
  // Specific behavior params
  float wave_amplitude{0.0f};
  float wave_frequency{0.0f};
  float detection_range{0.0f};
  // Weapon params
  bool can_shoot{false};
  float fire_rate{0.0f};
};

/**
 * @struct MissileConfig
 * @brief Data structure for projectile configuration
 */
struct MissileConfig {
  std::string name;
  std::uint32_t damage;
  float speed;  // Added speed here as it was missing in struct but present in
                // data
  float fire_rate_unused;  // Kept for schema compatibility if needed, but logic
                           // uses EnemyConfig.fire_rate usually
  float lifetime_seconds;
  float sprite_width;
  float sprite_height;
  float hitbox_scale;
  std::string texture_path;
  engine::render::Color tint_color;
};

/**
 * @struct ObstacleConfig
 * @brief Data structure for obstacle configuration
 */
struct ObstacleConfig {
  std::string name;
  bool destructible;
  std::uint32_t health;
  std::uint32_t score_value;
  float hitbox_scale;
  std::string texture_path;
  engine::render::Color tint_color;
};

/**
 * @struct WaveSpawnConfig
 * @brief Definition of a single spawn in a wave
 */
struct WaveSpawnConfig {
  float time;
  std::string enemy_type;
  float x;
  float y;
  bool random_y;
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
  static GameConfig& Get();

  /**
   * @brief Load all configuration files.
   * @param config_dir Path to the configuration directory (e.g. "config")
   * @return true if successful, false otherwise
   */
  bool Load(const std::string& config_dir);

  const WorldConfig& GetWorld() const { return world_config_; }
  const PlayerConfig& GetPlayer() const { return player_config_; }

  /**
   * @brief Get enemy configuration by name
   * @param name Enemy archetype name
   * @return Constant reference to EnemyConfig
   * @throw std::runtime_error if not found
   */
  const EnemyConfig& GetEnemy(const std::string& name) const;

  /**
   * @brief Get missile configuration by name
   * @param name Missile type name
   * @return Constant reference to MissileConfig
   * @throw std::runtime_error if not found
   */
  const MissileConfig& GetMissile(const std::string& name) const;

  /**
   * @brief Get obstacle configuration by name
   * @param name Obstacle type name
   * @return Constant reference to ObstacleConfig
   * @throw std::runtime_error if not found
   */
  const ObstacleConfig& GetObstacle(const std::string& name) const;

  /**
   * @brief Get level configuration by ID
   * @param id Level ID
   * @return Constant reference to LevelConfig
   * @throw std::runtime_error if not found
   */
  const LevelConfig& GetLevel(int id) const;

 private:
  GameConfig() = default;
  ~GameConfig() = default;

  GameConfig(const GameConfig&) = delete;
  GameConfig& operator=(const GameConfig&) = delete;

  WorldConfig world_config_;
  PlayerConfig player_config_;
  std::unordered_map<std::string, EnemyConfig> enemies_;
  std::unordered_map<std::string, MissileConfig> missiles_;
  std::unordered_map<std::string, ObstacleConfig> obstacles_;
  std::unordered_map<int, LevelConfig> levels_;
};

}  // namespace game_logic

#endif  // GAME_LOGIC_GAME_CONFIG_H_
