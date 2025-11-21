#include "ecs.h"
#include "oop.h"
#include <chrono>
#include <iomanip>
#include <iostream>

void RunOOPDemo() {
  std::cout << "OOP DEMO (1000 entities, 100 frames)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  OOPGameWorld world;

  for (int i = 0; i < 1000; ++i) {
    if (i % 2 == 0)
      world.AddEntity(std::make_unique<Player>());
    else
      world.AddEntity(std::make_unique<Enemy>());
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int frame = 0; frame < 100; ++frame) {
    world.UpdatePhysics(0.016f);
    world.UpdateHealth(0.016f);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Entities:        " << world.EntityCount() << "\n";
  std::cout << "Frames:          100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per frame:       " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "Per entity:      " << duration_us / 100000.0 << " μs\n";
  std::cout << "\n";
}

void RunECSDemo() {
  std::cout << "ECS DEMO (1000 entities, 100 frames)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  ECSGameWorld world;

  for (int i = 0; i < 1000; ++i) {
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

  auto start = std::chrono::high_resolution_clock::now();

  for (int frame = 0; frame < 100; ++frame) {
    world.Update(0.016f);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Entities:        " << world.EntityCount() << "\n";
  std::cout << "Frames:          100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per frame:       " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "Per entity:      " << duration_us / 100000.0 << " μs\n";
  std::cout << "\n";
}

int main() {
  RunOOPDemo();
  RunECSDemo();

  return 0;
}