#include "game_logic/game_config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace game_logic {

using json = nlohmann::json;

GameConfig& GameConfig::Get() {
  static GameConfig instance;
  return instance;
}

namespace {
engine::render::Color ParseColor(const json& j) {
  if (j.is_array() && j.size() == 4) {
    return engine::render::Color::FromBytes(j[0], j[1], j[2], j[3]);
  }
  return engine::render::Color::White();
}
}  // namespace

bool GameConfig::Load(const std::string& config_dir) {
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

    auto& w = global_j["world"];
    world_config_.grid_cell_size = w.value("grid_cell_size", 100.0f);
    world_config_.player_spawn_base_x = w.value("player_spawn_base_x", 100.0f);
    world_config_.player_spawn_offset_x =
        w.value("player_spawn_offset_x", 50.0f);
    world_config_.player_spawn_y = w.value("player_spawn_y", 300.0f);
    world_config_.spawn_min_y = w.value("spawn_min_y", 50.0f);
    world_config_.spawn_max_y = w.value("spawn_max_y", 700.0f);

    auto& b = global_j["behavior"];
    world_config_.patrol_min_x = b.value("patrol_min_x", 0.0f);
    world_config_.patrol_min_y = b.value("patrol_min_y", 100.0f);
    world_config_.patrol_max_x = b.value("patrol_max_x", 800.0f);
    world_config_.patrol_max_y = b.value("patrol_max_y", 500.0f);

    std::ifstream player_file(found_dir + "/player.json");
    if (player_file.is_open()) {
      json player_j;
      player_file >> player_j;
      auto& p = player_j["default"];
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
      for (auto& [key, val] : enemies_j.items()) {
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
      for (auto& [key, val] : missiles_j.items()) {
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
      for (auto& [key, val] : obstacles_j.items()) {
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

    if (std::filesystem::exists(found_dir + "/levels")) {
      for (const auto& entry :
           std::filesystem::directory_iterator(found_dir + "/levels")) {
        if (entry.path().extension() == ".json") {
          std::ifstream level_file(entry.path());
          json level_j;
          level_file >> level_j;

          LevelConfig level;
          level.id = level_j.value("id", 0);

          if (level_j.contains("waves") && level_j["waves"].is_array()) {
            for (const auto& w_j : level_j["waves"]) {
              WaveSpawnConfig spawn;
              spawn.time = w_j.value("time", 0.0f);
              spawn.enemy_type = w_j.value("type", "Scout");
              spawn.x = w_j.value("x", 0.0f);
              spawn.y = w_j.value("y", 0.0f);
              spawn.random_y = w_j.value("random_y", false);
              level.waves.push_back(spawn);
            }
          }
          levels_[level.id] = level;
        }
      }
    }

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Config Load Error: " << e.what() << std::endl;
    return false;
  }
}

const EnemyConfig& GameConfig::GetEnemy(const std::string& name) const {
  auto it = enemies_.find(name);
  if (it != enemies_.end()) return it->second;
  throw std::runtime_error("Config Error: Enemy '" + name + "' not found");
}

const MissileConfig& GameConfig::GetMissile(const std::string& name) const {
  auto it = missiles_.find(name);
  if (it != missiles_.end()) return it->second;
  throw std::runtime_error("Config Error: Missile '" + name + "' not found");
}

const ObstacleConfig& GameConfig::GetObstacle(const std::string& name) const {
  auto it = obstacles_.find(name);
  if (it != obstacles_.end()) return it->second;
  throw std::runtime_error("Config Error: Obstacle '" + name + "' not found");
}

const LevelConfig& GameConfig::GetLevel(int id) const {
  auto it = levels_.find(id);
  if (it != levels_.end()) return it->second;
  throw std::runtime_error("Config Error: Level " + std::to_string(id) +
                           " not found");
}

}  // namespace game_logic
