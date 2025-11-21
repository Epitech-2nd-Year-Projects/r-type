#include "ecs.h"
#include "oop.h"
#include <benchmark/benchmark.h>

static void BM_OOP_Physics_Only(benchmark::State &state) {
  const int count = state.range(0);
  OOPGameWorld world;

  for (int i = 0; i < count; ++i) {
    if (i % 2 == 0)
      world.AddEntity(std::make_unique<Player>());
    else
      world.AddEntity(std::make_unique<Enemy>());
  }

  for (auto _ : state) {
    world.UpdatePhysics(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_OOP_Both_Systems(benchmark::State &state) {
  const int count = state.range(0);
  OOPGameWorld world;

  for (int i = 0; i < count; ++i) {
    if (i % 2 == 0)
      world.AddEntity(std::make_unique<Player>());
    else
      world.AddEntity(std::make_unique<Enemy>());
  }

  for (auto _ : state) {
    world.UpdatePhysics(0.016f);
    world.UpdateHealth(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_ECS_Physics_Only(benchmark::State &state) {
  const int count = state.range(0);
  ECSGameWorld world;

  for (int i = 0; i < count; ++i) {
    world.positions[i] = {static_cast<float>(i), static_cast<float>(i)};
    world.velocities[i] = {1.0f, 1.5f};
  }

  world.AddSystem([](ECSGameWorld &w) {
    for (size_t i = 0; i < w.positions.size(); ++i) {
      auto &pos = w.positions[i];
      auto &vel = w.velocities[i];

      if (pos.has_value() && vel.has_value()) {
        pos.value().x += vel.value().vx * 0.016f;
        pos.value().y += vel.value().vy * 0.016f;
      }
    }
  });

  for (auto _ : state) {
    world.Update(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_ECS_Both_Systems(benchmark::State &state) {
  const int count = state.range(0);
  ECSGameWorld world;

  for (int i = 0; i < count; ++i) {
    world.positions[i] = {static_cast<float>(i), static_cast<float>(i)};
    world.velocities[i] = {1.0f, 1.5f};
    if (i % 3 == 0)
      world.healths[i] = {100, 100};
  }

  world.AddSystem([](ECSGameWorld &w) {
    for (size_t i = 0; i < w.positions.size(); ++i) {
      auto &pos = w.positions[i];
      auto &vel = w.velocities[i];

      if (pos.has_value() && vel.has_value()) {
        pos.value().x += vel.value().vx * 0.016f;
        pos.value().y += vel.value().vy * 0.016f;
      }
    }
  });

  world.AddSystem([](ECSGameWorld &w) {
    for (size_t i = 0; i < w.healths.size(); ++i) {
      auto &health = w.healths[i];
      if (health.has_value()) {
        health.value().hp--;
        if (health.value().hp < 0)
          health.value().hp = 0;
      }
    }
  });

  for (auto _ : state) {
    world.Update(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_OOP_Physics_Only)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_OOP_Both_Systems)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ECS_Physics_Only)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ECS_Both_Systems)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
