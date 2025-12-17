#include "game_logic/systems/wave_system.h"

#include <vector>

#include "engine/ecs/components/tag_component.h"
#include "game_logic/constants.h"
#include "game_logic/entities/enemy_builder.h"
#include "game_logic/game_config.h"
#include "game_logic/game_instance.h"

namespace game_logic::systems {

WaveSystem::WaveSystem(GameInstance &game_instance)
    : game_instance_(game_instance), rng_(std::random_device{}()) {
  LoadLevel(1);
}

void WaveSystem::LoadLevel(int level_id) {
  current_level_ = level_id;
  current_wave_time_ = 0.0f;
  pending_spawns_.clear();
  waiting_for_next_level_ = false;
  level_finished_timer_ = 0.0f;
  game_instance_.State().current_level = static_cast<std::uint32_t>(level_id);
  game_instance_.State().current_wave = 1u;

  try {
    const LevelConfig &level = GameConfig::Get().GetLevel(level_id);
    for (const auto &spawn : level.waves) {
      entities::EnemyType type = entities::EnemyType::kScout;
      if (spawn.enemy_type == "Bomber")
        type = entities::EnemyType::kBomber;
      else if (spawn.enemy_type == "Tank")
        type = entities::EnemyType::kTank;
      else if (spawn.enemy_type == "Interceptor")
        type = entities::EnemyType::kInterceptor;
      else if (spawn.enemy_type != "Scout") {
        std::cerr << "Warning: Unknown enemy type '" << spawn.enemy_type
                  << "' in wave config. Defaulting to Scout." << std::endl;
      }
      WaveEntry entry;
      entry.spawn_time = spawn.time;
      entry.type = type;
      entry.position = {spawn.x, spawn.y};
      entry.random_y = spawn.random_y;
      entry.drops_powerup = spawn.drops_powerup;
      pending_spawns_.push_back(entry);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error loading level " << level_id << ": " << e.what()
              << std::endl;
  }
}

void WaveSystem::Update(engine::ecs::Registry &registry,
                        engine::time::TimeDelta dt) {
  current_wave_time_ += dt.as_seconds();

  const auto &world = GameConfig::Get().GetWorld();
  std::uniform_real_distribution<float> dist(world.spawn_min_y,
                                             world.spawn_max_y);

  while (!pending_spawns_.empty()) {
    const auto &next_spawn = pending_spawns_.front();
    if (current_wave_time_ >= next_spawn.spawn_time) {
      engine::math::Vector2f spawn_pos = next_spawn.position;

      if (next_spawn.random_y) {
        spawn_pos.y = dist(rng_);
      }

      game_logic::entities::EnemySpawnConfig enemy_config;
      enemy_config.type = next_spawn.type;
      enemy_config.spawn_position = spawn_pos;
      enemy_config.drops_powerup = next_spawn.drops_powerup;

      game_logic::entities::EnemyBuilder::Create(registry, enemy_config);
      pending_spawns_.pop_front();

    } else {
      break;
    }
  }

  if (pending_spawns_.empty()) {
    bool enemies_alive = false;
    auto &tags = registry.GetComponents<engine::ecs::TagComponent>();
    for (const auto &tag : tags) {
      if (tag.has_value() && tag->tag == "Enemy") {
        enemies_alive = true;
        break;
      }
    }

    if (!enemies_alive) {
      if (!waiting_for_next_level_) {
        waiting_for_next_level_ = true;
        level_finished_timer_ = 0.0f;
      } else {
        level_finished_timer_ += dt.as_seconds();
        if (level_finished_timer_ >= kLevelTransitionDelay) {
          int next_level = current_level_ + 1;
          try {
            GameConfig::Get().GetLevel(next_level);
          } catch (...) {
            std::cerr << "Max level reached (" << current_level_
                      << "), looping back to level 1" << std::endl;
            next_level = 1;
          }
          try {
            LoadLevel(next_level);
            game_instance_.State().current_level = next_level;
          } catch (const std::exception &e) {
            std::cerr << "Level transition error: " << e.what() << std::endl;
            game_instance_.State().is_game_over = true;
          } catch (...) {
            std::cerr << "Unknown error during level transition" << std::endl;
            game_instance_.State().is_game_over = true;
          }
        }
      }
    }
  }
}

}  // namespace game_logic::systems
