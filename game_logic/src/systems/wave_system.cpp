#include "game_logic/systems/wave_system.h"

#include <vector>

#include "game_logic/constants.h"
#include "game_logic/entities/enemy_builder.h"
#include "game_logic/game_config.h"

namespace game_logic::systems {

WaveSystem::WaveSystem() : rng_(std::random_device{}()) { LoadLevel(1); }

void WaveSystem::LoadLevel(int level_id) {
  current_level_ = level_id;
  current_wave_time_ = 0.0f;
  pending_spawns_.clear();

  try {
    const LevelConfig& level = GameConfig::Get().GetLevel(level_id);
    for (const auto& spawn : level.waves) {
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

      pending_spawns_.push_back(
          {spawn.time, type, {spawn.x, spawn.y}, spawn.random_y});
    }
  } catch (const std::exception& e) {
    std::cerr << "Error loading level " << level_id << ": " << e.what()
              << std::endl;
  }
}

void WaveSystem::Update(engine::ecs::Registry& registry,
                        engine::time::TimeDelta dt) {
  current_wave_time_ += dt.as_seconds();

  const auto& world = GameConfig::Get().GetWorld();
  std::uniform_real_distribution<float> dist(world.spawn_min_y,
                                             world.spawn_max_y);

  while (!pending_spawns_.empty()) {
    const auto& next_spawn = pending_spawns_.front();
    if (current_wave_time_ >= next_spawn.spawn_time) {
      engine::math::Vector2f spawn_pos = next_spawn.position;

      if (next_spawn.random_y) {
        spawn_pos.y = dist(rng_);
      }

      game_logic::entities::EnemyBuilder::Create(registry, next_spawn.type,
                                                 spawn_pos);
      pending_spawns_.pop_front();
    } else {
      break;
    }
  }
}

}  // namespace game_logic::systems
