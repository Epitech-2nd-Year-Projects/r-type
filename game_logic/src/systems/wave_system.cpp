#include "game_logic/systems/wave_system.h"

#include <vector>

#include "game_logic/constants.h"
#include "game_logic/entities/enemy_builder.h"

namespace game_logic::systems {

WaveSystem::WaveSystem() : rng_(std::random_device{}()) { LoadLevel(1); }

namespace {

struct SpawnDef {
  float time;
  entities::EnemyType type;
  float x;
  float y;
  bool random_y;
};

const std::vector<SpawnDef> kLevel1Data = {
    {2.0f, entities::EnemyType::kScout, 850.0f, 0.0f, true},
    {3.0f, entities::EnemyType::kScout, 850.0f, 0.0f, true},
    {4.0f, entities::EnemyType::kScout, 850.0f, 0.0f, true},
    {8.0f, entities::EnemyType::kBomber, 900.0f, 0.0f, true},
    {8.5f, entities::EnemyType::kBomber, 900.0f, 0.0f, true},
    {9.0f, entities::EnemyType::kBomber, 900.0f, 0.0f, true},
    {12.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {12.5f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {13.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {15.0f, entities::EnemyType::kScout, 900.0f, 0.0f, true},
    {15.0f, entities::EnemyType::kScout, 900.0f, 0.0f, true},
    {16.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {20.0f, entities::EnemyType::kBomber, 900.0f, 0.0f, true},
    {20.0f, entities::EnemyType::kBomber, 900.0f, 0.0f, true},
    {22.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {22.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
    {24.0f, entities::EnemyType::kInterceptor, 900.0f, 0.0f, true},
};

}  // namespace

void WaveSystem::LoadLevel(int level_id) {
  current_level_ = level_id;
  current_wave_time_ = 0.0f;
  pending_spawns_.clear();

  const std::vector<SpawnDef>* data = nullptr;
  if (current_level_ == 1) {
    data = &kLevel1Data;
  }

  if (data) {
    for (const auto& spawn : *data) {
      pending_spawns_.push_back(
          {spawn.time, spawn.type, {spawn.x, spawn.y}, spawn.random_y});
    }
  }
}

void WaveSystem::Update(engine::ecs::Registry& registry,
                        engine::time::TimeDelta dt) {
  current_wave_time_ += dt.as_seconds();

  std::uniform_real_distribution<float> dist(game_logic::kSpawnMinY,
                                             game_logic::kSpawnMaxY);

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
