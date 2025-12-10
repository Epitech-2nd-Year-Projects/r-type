#include "game_logic/systems/wave_system.h"

#include "game_logic/entities/enemy_builder.h"

namespace game_logic::systems {

WaveSystem::WaveSystem() { LoadLevel(1); }

void WaveSystem::LoadLevel(int level_id) {
  current_level_ = level_id;
  current_wave_time_ = 0.0f;
  pending_spawns_.clear();

  if (current_level_ == 1) {
    pending_spawns_.push_back(
        {2.0f, entities::EnemyType::kScout, {850.0f, 100.0f}});
    pending_spawns_.push_back(
        {3.0f, entities::EnemyType::kScout, {850.0f, 300.0f}});
    pending_spawns_.push_back(
        {4.0f, entities::EnemyType::kScout, {850.0f, 500.0f}});

    pending_spawns_.push_back(
        {8.0f, entities::EnemyType::kBomber, {900.0f, 200.0f}});
    pending_spawns_.push_back(
        {8.5f, entities::EnemyType::kBomber, {900.0f, 400.0f}});
    pending_spawns_.push_back(
        {9.0f, entities::EnemyType::kBomber, {900.0f, 200.0f}});

    pending_spawns_.push_back(
        {12.0f, entities::EnemyType::kInterceptor, {900.0f, 100.0f}});
    pending_spawns_.push_back(
        {12.5f, entities::EnemyType::kInterceptor, {900.0f, 300.0f}});
    pending_spawns_.push_back(
        {13.0f, entities::EnemyType::kInterceptor, {900.0f, 500.0f}});

    pending_spawns_.push_back(
        {15.0f, entities::EnemyType::kScout, {900.0f, 100.0f}});
    pending_spawns_.push_back(
        {15.0f, entities::EnemyType::kScout, {900.0f, 500.0f}});
    pending_spawns_.push_back(
        {16.0f, entities::EnemyType::kInterceptor, {900.0f, 300.0f}});

    pending_spawns_.push_back(
        {20.0f, entities::EnemyType::kBomber, {900.0f, 150.0f}});
    pending_spawns_.push_back(
        {20.0f, entities::EnemyType::kBomber, {900.0f, 450.0f}});
    pending_spawns_.push_back(
        {22.0f, entities::EnemyType::kInterceptor, {900.0f, 100.0f}});
    pending_spawns_.push_back(
        {22.0f, entities::EnemyType::kInterceptor, {900.0f, 500.0f}});
    pending_spawns_.push_back(
        {24.0f, entities::EnemyType::kInterceptor, {900.0f, 300.0f}});
  }
}

void WaveSystem::Update(engine::ecs::Registry &registry,
                        engine::time::TimeDelta dt) {
  current_wave_time_ += dt.as_seconds();

  while (!pending_spawns_.empty()) {
    const auto &next_spawn = pending_spawns_.front();
    if (current_wave_time_ >= next_spawn.spawn_time) {
      game_logic::entities::EnemyBuilder::Create(registry, next_spawn.type,
                                                 next_spawn.position);
      pending_spawns_.pop_front();
    } else {
      break;
    }
  }
}

}  // namespace game_logic::systems
