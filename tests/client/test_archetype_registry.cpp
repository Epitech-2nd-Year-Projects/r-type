#include <gtest/gtest.h>

#include <optional>

#include "ecs/archetype_registry.h"

TEST(ArchetypeRegistryTest, ClassifiesTypeCodes) {
  const auto& registry = client::ecs::ArchetypeRegistry::Get();

  EXPECT_EQ(registry.KindOf(1u), client::ecs::ArchetypeKind::kPlayer);
  EXPECT_EQ(registry.KindOf(2u), client::ecs::ArchetypeKind::kEnemy);
  EXPECT_EQ(registry.KindOf(3u), client::ecs::ArchetypeKind::kMissile);
  EXPECT_EQ(registry.KindOf(4u), client::ecs::ArchetypeKind::kObstacle);
  EXPECT_EQ(registry.KindOf(5u), client::ecs::ArchetypeKind::kPowerup);
  EXPECT_EQ(registry.KindOf(999u), client::ecs::ArchetypeKind::kUnknown);

  EXPECT_TRUE(registry.IsDamageable(1u));
  EXPECT_TRUE(registry.IsDamageable(2u));
  EXPECT_FALSE(registry.IsDamageable(3u));
  EXPECT_TRUE(registry.IsExplosive(3u));
  EXPECT_FALSE(registry.IsExplosive(4u));
}

TEST(ArchetypeRegistryTest, ResolvesPlayerSpriteTint) {
  const auto& registry = client::ecs::ArchetypeRegistry::Get();
  client::ecs::SpriteContext context{};
  context.network_id = 2u;

  const auto sprite = registry.ResolveSprite(1u, context);

  ASSERT_TRUE(sprite.has_value());
  EXPECT_EQ(sprite->texture_id, "assets/sprites/player.png");
  EXPECT_EQ(sprite->layer, 10);
  EXPECT_EQ(sprite->tint, engine::render::Color::FromBytes(60, 255, 60));
}

TEST(ArchetypeRegistryTest, ResolvesEnemySpriteByHealth) {
  const auto& registry = client::ecs::ArchetypeRegistry::Get();
  client::ecs::SpriteContext context{};
  context.health = client::ecs::HealthComponent(50u, 150u);

  const auto sprite = registry.ResolveSprite(2u, context);

  ASSERT_TRUE(sprite.has_value());
  EXPECT_EQ(sprite->texture_id, "assets/sprites/enemy_tank.png");
  EXPECT_EQ(sprite->layer, 5);
}

TEST(ArchetypeRegistryTest, ResolvesMissileSpriteByVelocity) {
  const auto& registry = client::ecs::ArchetypeRegistry::Get();
  client::ecs::SpriteContext context{};
  context.velocity = client::ecs::VelocityComponent(-10.0f, 0.0f);

  const auto sprite = registry.ResolveSprite(3u, context);

  ASSERT_TRUE(sprite.has_value());
  EXPECT_EQ(sprite->texture_id, "assets/sprites/enemy_missile.png");
  EXPECT_TRUE(sprite->face_left);
}

TEST(ArchetypeRegistryTest, ResolvesObstacleSpriteByHealth) {
  const auto& registry = client::ecs::ArchetypeRegistry::Get();
  client::ecs::SpriteContext context{};
  context.health = client::ecs::HealthComponent(0u, 0u);

  const auto sprite = registry.ResolveSprite(4u, context);

  ASSERT_TRUE(sprite.has_value());
  EXPECT_EQ(sprite->texture_id, "assets/sprites/obstacle_wall.png");
}
