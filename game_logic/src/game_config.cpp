#include "game_logic/game_config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

namespace game_logic {

using json = nlohmann::json;

GameConfig &GameConfig::Get() {
  static GameConfig instance;
  return instance;
}

namespace {
engine::render::Color ParseColor(const json &j) {
  if (j.is_array() && j.size() == 4) {
    return engine::render::Color::FromBytes(j[0], j[1], j[2], j[3]);
  }
  return engine::render::Color::White();
}

components::PowerupType ParsePowerupType(const std::string &type_str) {
  if (type_str == "Health") return components::PowerupType::kHealth;
  if (type_str == "WeaponUpgrade")
    return components::PowerupType::kWeaponUpgrade;
  if (type_str == "SpeedBoost") return components::PowerupType::kSpeedBoost;
  if (type_str == "Shield") return components::PowerupType::kShield;
  if (type_str == "ExtraLife") return components::PowerupType::kExtraLife;
  if (type_str == "Score") return components::PowerupType::kScore;
  return components::PowerupType::kHealth;
}
}  // namespace

bool GameConfig::Load(const std::string &config_dir) {
  std::string found_dir = "../../../../config";
  if (!config_dir.empty() &&
      std::filesystem::exists(config_dir + "/global.json")) {
    found_dir = config_dir;
  }

  if (!std::filesystem::exists(found_dir + "/global.json")) {
    std::cerr << "Config Error: Could not find configuration directory."
              << std::endl;
    return false;
  }

  try {
    std::ifstream global_file(found_dir + "/global.json");
    if (!global_file.is_open()) return false;
    json global_j;
    global_file >> global_j;

    auto &w = global_j["world"];
    world_config_.grid_cell_size = w.value("grid_cell_size", 100.0f);
    world_config_.player_spawn_base_x = w.value("player_spawn_base_x", 100.0f);
    world_config_.player_spawn_offset_x =
        w.value("player_spawn_offset_x", 50.0f);
    world_config_.player_spawn_y = w.value("player_spawn_y", 300.0f);
    world_config_.spawn_min_y = w.value("spawn_min_y", 50.0f);
    world_config_.spawn_max_y = w.value("spawn_max_y", 700.0f);

    auto &b = global_j["behavior"];
    world_config_.patrol_min_x = b.value("patrol_min_x", 0.0f);
    world_config_.patrol_min_y = b.value("patrol_min_y", 100.0f);
    world_config_.patrol_max_x = b.value("patrol_max_x", 800.0f);
    world_config_.patrol_max_y = b.value("patrol_max_y", 500.0f);

    std::ifstream player_file(found_dir + "/player.json");
    if (player_file.is_open()) {
      json player_j;
      player_file >> player_j;
      auto &p = player_j["default"];
      player_config_.health = p.value("health", 100);
      player_config_.lives = p.value("lives", 3);
      player_config_.speed = p.value("speed", 200.0f);
      player_config_.hitbox_width = p.value("hitbox_width", 32.0f);
      player_config_.hitbox_height = p.value("hitbox_height", 16.0f);
      player_config_.sprite_width = p.value("sprite_width", 32.0f);
      player_config_.sprite_height = p.value("sprite_height", 16.0f);
      player_config_.texture_path = p.value("texture_path", "");
    }

    std::ifstream enemies_file(found_dir + "/enemies.json");
    if (enemies_file.is_open()) {
      json enemies_j;
      enemies_file >> enemies_j;
      for (auto &[key, val] : enemies_j.items()) {
        EnemyConfig c;
        c.name = val.value("name", key);
        c.health = val.value("health", 10);
        c.speed = val.value("speed", 100.0f);
        c.behavior_type = val.value("behavior", "Straight");
        c.score = val.value("score", 0);
        c.sprite_width = val.value("sprite_width", 32.0f);
        c.sprite_height = val.value("sprite_height", 32.0f);
        c.hitbox_width = val.value("hitbox_width", 32.0f);
        c.hitbox_height = val.value("hitbox_height", 32.0f);
        c.texture_path = val.value("texture_path", "");
        c.wave_amplitude = val.value("wave_amplitude", 0.0f);
        c.wave_frequency = val.value("wave_frequency", 0.0f);
        c.detection_range = val.value("detection_range", 0.0f);
        c.can_shoot = val.value("can_shoot", false);
        c.fire_rate = val.value("fire_rate", 0.0f);
        enemies_[key] = c;
      }
    }

    std::ifstream missiles_file(found_dir + "/missiles.json");
    if (missiles_file.is_open()) {
      json missiles_j;
      missiles_file >> missiles_j;
      for (auto &[key, val] : missiles_j.items()) {
        MissileConfig c;
        c.name = val.value("name", key);
        c.damage = val.value("damage", 10);
        c.speed = val.value("speed", 300.0f);
        c.lifetime_seconds = val.value("lifetime", 5.0f);
        c.sprite_width = val.value("sprite_width", 16.0f);
        c.sprite_height = val.value("sprite_height", 8.0f);
        c.hitbox_scale = val.value("hitbox_scale", 1.0f);
        c.texture_path = val.value("texture_path", "");
        if (val.contains("tint")) {
          c.tint_color = ParseColor(val["tint"]);
        } else {
          c.tint_color = engine::render::Color::White();
        }
        missiles_[key] = c;
      }
    }

    std::ifstream obstacles_file(found_dir + "/obstacles.json");
    if (obstacles_file.is_open()) {
      json obstacles_j;
      obstacles_file >> obstacles_j;
      for (auto &[key, val] : obstacles_j.items()) {
        ObstacleConfig c;
        c.name = val.value("name", key);
        c.destructible = val.value("destructible", false);
        c.health = val.value("health", 0);
        c.score_value = val.value("score_value", 0);
        c.hitbox_scale = val.value("hitbox_scale", 1.0f);
        c.texture_path = val.value("texture_path", "");
        if (val.contains("tint")) {
          c.tint_color = ParseColor(val["tint"]);
        } else {
          c.tint_color = engine::render::Color::White();
        }
        obstacles_[key] = c;
      }
    }

    std::ifstream powerups_file(found_dir + "/powerups.json");
    if (powerups_file.is_open()) {
      json powerups_j;
      powerups_file >> powerups_j;
      for (auto &[key, val] : powerups_j.items()) {
        PowerupConfig c;
        c.name = key;
        c.type = ParsePowerupType(val.value("type", "Health"));
        c.value = val.value("value", 0);
        c.duration = val.value("duration", 0.0f);
        c.drop_probability = val.value("drop_probability", 1.0f);
        c.sprite_width = val.value("sprite_width", 16.0f);
        c.sprite_height = val.value("sprite_height", 16.0f);
        c.texture_path = val.value("texture_path", "");
        powerups_.push_back(c);
      }
    }

    if (std::filesystem::exists(found_dir + "/levels")) {
      for (const auto &entry :
           std::filesystem::directory_iterator(found_dir + "/levels")) {
        if (entry.path().extension() == ".json") {
          std::ifstream level_file(entry.path());
          json level_j;
          level_file >> level_j;

          LevelConfig level;
          level.id = level_j.value("id", 0);

          if (level_j.contains("waves") && level_j["waves"].is_array()) {
            for (const auto &w_j : level_j["waves"]) {
              WaveSpawnConfig spawn;
              spawn.time = w_j.value("time", 0.0f);
              spawn.enemy_type = w_j.value("type", "Scout");
              spawn.x = w_j.value("x", 0.0f);
              spawn.y = w_j.value("y", 0.0f);
              spawn.random_y = w_j.value("random_y", false);
              spawn.drops_powerup = w_j.value("drops_powerup", false);
              level.waves.push_back(spawn);
            }
          }
          levels_[level.id] = level;
        }
      }
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Config Load Error: " << e.what() << std::endl;
    return false;
  }
}

const EnemyConfig &GameConfig::GetEnemy(const std::string &name) const {
  auto it = enemies_.find(name);
  if (it != enemies_.end()) return it->second;
  throw std::runtime_error("Config Error: Enemy '" + name + "' not found");
}

const MissileConfig &GameConfig::GetMissile(const std::string &name) const {
  auto it = missiles_.find(name);
  if (it != missiles_.end()) return it->second;
  throw std::runtime_error("Config Error: Missile '" + name + "' not found");
}

const ObstacleConfig &GameConfig::GetObstacle(const std::string &name) const {
  auto it = obstacles_.find(name);
  if (it != obstacles_.end()) return it->second;
  throw std::runtime_error("Config Error: Obstacle '" + name + "' not found");
}

const LevelConfig &GameConfig::GetLevel(int id) const {
  auto it = levels_.find(id);
  if (it != levels_.end()) return it->second;
  throw std::runtime_error("Config Error: Level " + std::to_string(id) +
                           " not found");
}

const PowerupConfig &GameConfig::GetRandomPowerup() const {
  if (powerups_.empty()) {
    throw std::runtime_error("No powerups defined in config");
  }

  static std::random_device rd;
  static std::mt19937 gen(rd());

  float total_weight = 0.0f;
  for (const auto &p : powerups_) {
    total_weight += p.drop_probability;
  }

  std::uniform_real_distribution<float> dist(0.0f, total_weight);
  float r = dist(gen);

  float cumulative_weight = 0.0f;
  for (const auto &p : powerups_) {
    cumulative_weight += p.drop_probability;
    if (r <= cumulative_weight) {
      return p;
    }
  }

  return powerups_.back();
}

}  // namespace game_logic
