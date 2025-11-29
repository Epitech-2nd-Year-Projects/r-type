#include <benchmark/benchmark.h>

#include "bench_types.h"
#include "ecs.h"
#include "oop.h"

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
    if (i % 3 == 0) world.healths[i] = {100, 100};
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
        if (health.value().hp < 0) health.value().hp = 0;
      }
    }
  });

  for (auto _ : state) {
    world.Update(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

// 4 systems (physics + health + AI + DOT) to show multi-system scaling.
static void BM_OOP_Four_Systems(benchmark::State &state) {
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
    world.UpdateAI(0.016f);
    world.UpdateDOT(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_ECS_Four_Systems(benchmark::State &state) {
  const int count = state.range(0);
  ECSGameWorld world;

  for (int i = 0; i < count; ++i) {
    world.positions[i] = {static_cast<float>(i), static_cast<float>(i)};
    world.velocities[i] = {1.0f, 1.5f};
    world.healths[i] = {100, 100};
    world.ai_states[i] = {1.0f, 0.0f};
    world.dots[i] = {1, 0.1f, 0.0f};
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
        if (health.value().hp < 0) health.value().hp = 0;
      }
    }
  });

  world.AddSystem([](ECSGameWorld &w) {
    for (size_t i = 0; i < w.ai_states.size(); ++i) {
      auto &ai = w.ai_states[i];
      auto &vel = w.velocities[i];
      if (!ai.has_value() || !vel.has_value()) continue;

      ai.value().timer += 0.016f;
      if (ai.value().timer >= 0.05f) {
        ai.value().direction = -ai.value().direction;
        vel.value().vx = -vel.value().vx;
        vel.value().vy = -vel.value().vy;
        ai.value().timer = 0.0f;
      }
    }
  });

  world.AddSystem([](ECSGameWorld &w) {
    for (size_t i = 0; i < w.healths.size(); ++i) {
      auto &health = w.healths[i];
      auto &dot = w.dots[i];
      if (!health.has_value() || !dot.has_value()) continue;

      dot.value().accumulator += 0.016f;
      if (dot.value().accumulator >= dot.value().interval) {
        health.value().hp -= dot.value().damage_per_tick;
        if (health.value().hp < 0) health.value().hp = 0;
        dot.value().accumulator = 0.0f;
      }
    }
  });

  for (auto _ : state) {
    world.Update(0.016f);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

// Spawn/despawn throughput: create and clear N entities.
static void BM_OOP_SpawnDespawn(benchmark::State &state) {
  const int count = state.range(0);

  for (auto _ : state) {
    OOPGameWorld world;

    for (int i = 0; i < count; ++i) {
      if (i % 2 == 0)
        world.AddEntity(std::make_unique<Player>());
      else
        world.AddEntity(std::make_unique<Enemy>());
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_ECS_SpawnDespawn(benchmark::State &state) {
  const int count = state.range(0);

  for (auto _ : state) {
    ECSGameWorld world;

    for (int i = 0; i < count; ++i) {
      world.positions[i] = {static_cast<float>(i), static_cast<float>(i)};
      world.velocities[i] = {1.0f, 1.5f};
      world.healths[i] = {100, 100};
    }

    for (int i = 0; i < count; ++i) {
      world.positions[i].reset();
      world.velocities[i].reset();
      world.healths[i].reset();
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * count);
}

// Serialization throughput to a contiguous buffer.
static void BM_OOP_Serialize(benchmark::State &state) {
  const int count = state.range(0);
  OOPGameWorld world;

  for (int i = 0; i < count; ++i) {
    if (i % 2 == 0)
      world.AddEntity(std::make_unique<Player>());
    else
      world.AddEntity(std::make_unique<Enemy>());
  }

  std::vector<SerializedEntity> buffer;

  for (auto _ : state) {
    world.SerializeAll(buffer);
    benchmark::DoNotOptimize(buffer.data());
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_ECS_Serialize(benchmark::State &state) {
  const int count = state.range(0);
  ECSGameWorld world;

  for (int i = 0; i < count; ++i) {
    world.positions[i] = {static_cast<float>(i), static_cast<float>(i)};
    world.velocities[i] = {1.0f, 1.5f};
    world.healths[i] = {100, 100};
  }

  std::vector<SerializedEntity> buffer;

  for (auto _ : state) {
    world.SerializeAll(buffer);
    benchmark::DoNotOptimize(buffer.data());
  }

  state.SetItemsProcessed(state.iterations() * count);
}

// Static footprint counters: recorded as benchmark counters.
static void BM_StaticFootprint(benchmark::State &state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(sizeof(Player));
    benchmark::DoNotOptimize(sizeof(Enemy));
    benchmark::DoNotOptimize(sizeof(Position));
    benchmark::DoNotOptimize(sizeof(Velocity));
    benchmark::DoNotOptimize(sizeof(Health));
    benchmark::DoNotOptimize(sizeof(AIState));
    benchmark::DoNotOptimize(sizeof(DamageOverTime));
    benchmark::DoNotOptimize(sizeof(std::unique_ptr<GameEntity>));
  }

  state.counters["Player_bytes"] = sizeof(Player);
  state.counters["Enemy_bytes"] = sizeof(Enemy);
  state.counters["Position_opt_bytes"] = sizeof(std::optional<Position>);
  state.counters["Velocity_opt_bytes"] = sizeof(std::optional<Velocity>);
  state.counters["Health_opt_bytes"] = sizeof(std::optional<Health>);
  state.counters["AIState_opt_bytes"] = sizeof(std::optional<AIState>);
  state.counters["DOT_opt_bytes"] = sizeof(std::optional<DamageOverTime>);
  state.counters["OOP_slot_bytes"] = sizeof(std::unique_ptr<GameEntity>);

  const double oop_entity = (sizeof(Player) + sizeof(Enemy)) / 2.0;
  const double ecs_entity = sizeof(Position) + sizeof(Velocity) +
                            sizeof(Health) + sizeof(AIState) +
                            sizeof(DamageOverTime);

  state.counters["Approx_OOP_bytes_per_entity"] = oop_entity;
  state.counters["Approx_ECS_bytes_per_entity"] = ecs_entity;
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

BENCHMARK(BM_OOP_Four_Systems)->Arg(10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ECS_Four_Systems)->Arg(10000)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_OOP_SpawnDespawn)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ECS_SpawnDespawn)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_OOP_Serialize)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ECS_Serialize)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_StaticFootprint);

BENCHMARK_MAIN();
