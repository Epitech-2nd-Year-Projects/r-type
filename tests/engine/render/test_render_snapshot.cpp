#include <gtest/gtest.h>

#include "engine/ecs/components/sprite_component.h"
#include "engine/ecs/components/transform_component.h"
#include "engine/ecs/registry.h"
#include "engine/render/render_snapshot.h"

TEST(RenderSnapshotTest, RoundTrip) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::SpriteComponent>();
  registry.RegisterComponent<engine::ecs::TransformComponent>();

  auto entity = registry.SpawnEntity();

  engine::ecs::SpriteComponent sprite("assets/test.png");
  sprite.layer = 5;
  sprite.tint = {255, 0, 0, 255};

  engine::ecs::TransformComponent transform;
  transform.transform.SetPosition({100.0f, 200.0f});
  transform.transform.SetRotation(45.0f);
  transform.transform.SetScale({2.0f, 2.0f});

  registry.AddComponent<engine::ecs::SpriteComponent>(entity, sprite);
  registry.AddComponent<engine::ecs::TransformComponent>(entity, transform);

  engine::render::RenderSnapshot snapshot =
      engine::render::ExtractSnapshot(registry, 42);

  EXPECT_TRUE(snapshot.valid);
  EXPECT_EQ(snapshot.tick, 42);
  EXPECT_GT(snapshot.timestamp_ns, 0);
  ASSERT_EQ(snapshot.sprites.size(), 1);

  const auto& data = snapshot.sprites[0];
  EXPECT_EQ(data.entity_id, static_cast<std::uint32_t>(entity));
  EXPECT_EQ(data.texture_path, "assets/test.png");
  EXPECT_EQ(data.layer, 5);
  EXPECT_EQ(data.tint.r, 255);
  EXPECT_EQ(data.tint.g, 0);
  EXPECT_EQ(data.position.x, 100.0f);
  EXPECT_EQ(data.position.y, 200.0f);
  EXPECT_FLOAT_EQ(data.rotation, 45.0f);
  EXPECT_FLOAT_EQ(data.scale.x, 2.0f);
}

TEST(RenderSnapshotTest, SkipInvisible) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::SpriteComponent>();
  registry.RegisterComponent<engine::ecs::TransformComponent>();

  auto entity = registry.SpawnEntity();
  engine::ecs::SpriteComponent sprite("visible.png");
  sprite.visible = false;
  registry.AddComponent<engine::ecs::SpriteComponent>(entity, sprite);
  registry.AddComponent<engine::ecs::TransformComponent>(
      entity, engine::ecs::TransformComponent{});

  auto visible_entity = registry.SpawnEntity();
  registry.AddComponent<engine::ecs::SpriteComponent>(
      visible_entity, engine::ecs::SpriteComponent{"visible.png"});
  registry.AddComponent<engine::ecs::TransformComponent>(
      visible_entity, engine::ecs::TransformComponent{});

  auto snapshot = engine::render::ExtractSnapshot(registry, 10);
  ASSERT_EQ(snapshot.sprites.size(), 1);
  EXPECT_EQ(snapshot.sprites[0].entity_id,
            static_cast<std::uint32_t>(visible_entity));
}
