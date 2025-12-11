#include <gtest/gtest.h>

#include "ecs/interpolation_system.h"
#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"

namespace {

using client::ecs::InterpolationSystem;
using client::ecs::PositionComponent;
using client::ecs::SnapshotInterpolationComponent;
using client::ecs::VelocityComponent;

TEST(InterpolationSystemTest, InterpolatesBetweenSnapshots) {
  engine::ecs::Registry registry;
  InterpolationSystem system(registry);

  const auto entity = registry.SpawnEntity();
  auto& positions = registry.GetComponents<PositionComponent>();
  positions[entity] = PositionComponent({0.0f, 0.0f}, {0.0f, 0.0f});
  positions[entity]->previous_position = {0.0f, 0.0f};
  positions[entity]->position = {10.0f, 0.0f};

  auto& snapshots = registry.GetComponents<SnapshotInterpolationComponent>();
  snapshots[entity] = SnapshotInterpolationComponent(200, 100);

  system.SetInterpolationDelayMs(0);
  system.SetMaxExtrapolationMs(0);
  system.UpdateAt(engine::time::TimeDelta::zero(), 150);

  ASSERT_TRUE(positions[entity].has_value());
  EXPECT_NEAR(positions[entity]->render_position.x, 5.0f, 0.01f);
  EXPECT_NEAR(positions[entity]->render_position.y, 0.0f, 0.01f);
}

TEST(InterpolationSystemTest, ExtrapolatesWithVelocityWhenStale) {
  engine::ecs::Registry registry;
  InterpolationSystem system(registry);

  const auto entity = registry.SpawnEntity();
  auto& positions = registry.GetComponents<PositionComponent>();
  positions[entity] = PositionComponent({0.0f, 0.0f}, {0.0f, 0.0f});
  positions[entity]->previous_position = {0.0f, 0.0f};
  positions[entity]->position = {10.0f, -2.0f};
  positions[entity]->render_position = positions[entity]->position;

  auto& velocities = registry.GetComponents<VelocityComponent>();
  velocities[entity] = VelocityComponent(engine::math::Vector2f{2.0f, 4.0f});

  auto& snapshots = registry.GetComponents<SnapshotInterpolationComponent>();
  snapshots[entity] = SnapshotInterpolationComponent(200, 100);

  system.SetInterpolationDelayMs(0);
  system.SetMaxExtrapolationMs(1000);
  system.UpdateAt(engine::time::TimeDelta::zero(), 260);

  ASSERT_TRUE(positions[entity].has_value());
  // 60 ms beyond the last snapshot with velocity applied
  EXPECT_NEAR(positions[entity]->render_position.x, 10.12f, 0.01f);
  EXPECT_NEAR(positions[entity]->render_position.y, -1.76f, 0.01f);
}

}  // namespace
