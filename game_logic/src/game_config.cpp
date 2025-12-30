#include "game_logic/game_config.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>

#include "engine/util/logging.h"

namespace game_logic {

using json = nlohmann::json;

GameConfig &GameConfig::Get() {
  static GameConfig instance;
  return instance;
}

namespace {}  // namespace

bool GameConfig::Load(const std::string &config_dir) {
  auto &logger = engine::util::Logger::Default();
  std::string found_dir = "../../../../config";
  if (!config_dir.empty() &&
      std::filesystem::exists(config_dir + "/global.json")) {
    found_dir = config_dir;
  }
  config_dir_ = found_dir;

  if (!std::filesystem::exists(found_dir + "/global.json")) {
    logger.Error(
        "[game_logic.config] Config Error: Could not find configuration "
        "directory");
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
    logger.Error("[game_logic.config] Config Load Error: ", e.what());
    return false;
  }
}

const LevelConfig &GameConfig::GetLevel(int id) const {
  auto it = levels_.find(id);
  if (it != levels_.end()) return it->second;
  throw std::runtime_error("Config Error: Level " + std::to_string(id) +
                           " not found");
}

}  // namespace game_logic
