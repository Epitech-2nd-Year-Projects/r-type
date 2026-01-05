#include <gtest/gtest.h>

#include "ecs/archetype_registry.h"
#include "ecs/components.h"
#include "ecs/render_debug.h"
#include "engine/ecs/registry.h"
#include "fake_renderer.h"
#include "systems/debug_path_system.h"

TEST(DebugPathSystemTest, DrawsNothingWhenDisabled) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::PositionComponent>();
  registry.RegisterComponent<client::ecs::VelocityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();
  registry.RegisterComponent<client::ecs::LocalPlayerTag>();
  registry.RegisterComponent<client::ecs::SpriteComponent>();

  FakeRenderer fake_renderer;
  client::ecs::RenderDebug render_debug(registry, fake_renderer);
  client::systems::DebugPathSystem system(registry, render_debug);

  system.Update(engine::time::TimeDelta::from_seconds(0.016));

  EXPECT_TRUE(fake_renderer.line_calls.empty());
}

TEST(DebugPathSystemTest, DrawsPathsForScoutWhenEnabled) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::PositionComponent>();
  registry.RegisterComponent<client::ecs::VelocityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();
  registry.RegisterComponent<client::ecs::LocalPlayerTag>();
  registry.RegisterComponent<client::ecs::SpriteComponent>();

  FakeRenderer fake_renderer;
  client::ecs::RenderDebug render_debug(registry, fake_renderer);
  render_debug.show_ai_paths = true;
  client::systems::DebugPathSystem system(registry, render_debug);

  auto entity = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(entity, 1u,
                                                                   2u, 1u);
  registry.EmplaceComponent<client::ecs::PositionComponent>(entity, 100.0f,
                                                            100.0f);
  registry.EmplaceComponent<client::ecs::VelocityComponent>(entity, 50.0f,
                                                            0.0f);
  system.Update(engine::time::TimeDelta::from_seconds(0.016));

  EXPECT_FALSE(fake_renderer.line_calls.empty());
  EXPECT_EQ(fake_renderer.line_calls[0].color.g, 1.0f);
  EXPECT_EQ(fake_renderer.line_calls[0].color.b, 1.0f);
}

TEST(DebugPathSystemTest, SkipsMissiles) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::PositionComponent>();
  registry.RegisterComponent<client::ecs::VelocityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();
  registry.RegisterComponent<client::ecs::LocalPlayerTag>();
  registry.RegisterComponent<client::ecs::SpriteComponent>();

  FakeRenderer fake_renderer;
  client::ecs::RenderDebug render_debug(registry, fake_renderer);
  render_debug.show_ai_paths = true;
  client::systems::DebugPathSystem system(registry, render_debug);

  auto entity = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(entity, 1u,
                                                                   3u, 1u);
  registry.EmplaceComponent<client::ecs::PositionComponent>(entity, 100.0f,
                                                            100.0f);
  registry.EmplaceComponent<client::ecs::VelocityComponent>(entity, 50.0f,
                                                            0.0f);

  system.Update(engine::time::TimeDelta::from_seconds(0.016));

  EXPECT_TRUE(fake_renderer.line_calls.empty());
}

TEST(DebugPathSystemTest, DrawsWaveForBomber) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::PositionComponent>();
  registry.RegisterComponent<client::ecs::VelocityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();
  registry.RegisterComponent<client::ecs::LocalPlayerTag>();
  registry.RegisterComponent<client::ecs::SpriteComponent>();

  FakeRenderer fake_renderer;
  client::ecs::RenderDebug render_debug(registry, fake_renderer);
  render_debug.show_ai_paths = true;
  client::systems::DebugPathSystem system(registry, render_debug);

  auto entity = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(entity, 1u,
                                                                   2u, 1u);
  registry.EmplaceComponent<client::ecs::PositionComponent>(entity, 100.0f,
                                                            100.0f);
  registry.EmplaceComponent<client::ecs::VelocityComponent>(entity, 50.0f,
                                                            0.0f);
  registry.EmplaceComponent<client::ecs::HealthComponent>(entity, 20u);

  system.Update(engine::time::TimeDelta::from_seconds(0.016));

  ASSERT_FALSE(fake_renderer.line_calls.empty());
  EXPECT_EQ(fake_renderer.line_calls[0].color.r, 1.0f);
  EXPECT_EQ(fake_renderer.line_calls[0].color.b, 1.0f);
  EXPECT_EQ(fake_renderer.line_calls[0].color.g, 0.0f);
}
