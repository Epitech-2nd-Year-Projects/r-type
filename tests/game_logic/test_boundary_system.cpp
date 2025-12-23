#include <gtest/gtest.h>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"
#include "game_logic/components/player_component.h"
#include "game_logic/systems/boundary_system.h"

using namespace game_logic::systems;

TEST(BoundarySystemTest, ClampsPlayerPositionToScreen) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();
  registry.RegisterComponent<engine::ecs::VelocityComponent>();
  registry.RegisterComponent<game_logic::components::PlayerComponent>();

  engine::ecs::EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, -50.0f,
                                                            -50.0f);
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(entity, 0.0f, 0.0f);
  registry.EmplaceComponent<game_logic::components::PlayerComponent>(entity);

  BoundarySystem boundarySystem(1920.0f, 1080.0f);
  boundarySystem.Update(registry, engine::time::TimeDelta::from_seconds(1.0f));

  auto& pos = registry.GetComponents<engine::ecs::PositionComponent>()[entity];
  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->position.x, 0.0f);
  EXPECT_FLOAT_EQ(pos->position.y, 0.0f);
}

TEST(BoundarySystemTest, ClampsPlayerPositionToScreenMax) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();
  registry.RegisterComponent<engine::ecs::VelocityComponent>();
  registry.RegisterComponent<game_logic::components::PlayerComponent>();

  engine::ecs::EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, 2000.0f,
                                                            1200.0f);
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(entity, 0.0f, 0.0f);
  registry.EmplaceComponent<game_logic::components::PlayerComponent>(entity);

  BoundarySystem boundarySystem(1920.0f, 1080.0f);
  boundarySystem.Update(registry, engine::time::TimeDelta::from_seconds(1.0f));

  auto& pos = registry.GetComponents<engine::ecs::PositionComponent>()[entity];
  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->position.x, 1920.0f);
  EXPECT_FLOAT_EQ(pos->position.y, 1080.0f);
}

TEST(BoundarySystemTest, KillsEntityOffScreen) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();
  registry.RegisterComponent<game_logic::components::PlayerComponent>();

  engine::ecs::EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, -250.0f,
                                                            100.0f);

  BoundarySystem boundarySystem(1920.0f, 1080.0f);
  boundarySystem.Update(registry, engine::time::TimeDelta::from_seconds(1.0f));

  EXPECT_FALSE(registry.IsAlive(entity));
}

TEST(BoundarySystemTest, DoesNotKillPlayerOffScreen) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();
  registry.RegisterComponent<game_logic::components::PlayerComponent>();

  engine::ecs::EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, -250.0f,
                                                            100.0f);
  registry.EmplaceComponent<game_logic::components::PlayerComponent>(entity);

  BoundarySystem boundarySystem(1920.0f, 1080.0f);
  boundarySystem.Update(registry, engine::time::TimeDelta::from_seconds(1.0f));

  EXPECT_TRUE(registry.IsAlive(entity));
}
