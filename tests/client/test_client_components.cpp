#include <gtest/gtest.h>

#include "ecs/components.h"

namespace {

TEST(ClientComponentsTest, DefaultsAreInitialized) {
  client::ecs::NetworkedEntityComponent net{};
  EXPECT_EQ(net.network_id, 0u);
  EXPECT_EQ(net.type_code, 0u);
  EXPECT_EQ(net.last_snapshot, 0u);

  client::ecs::PositionComponent pos{};
  EXPECT_FLOAT_EQ(pos.position.x, 0.0f);
  EXPECT_FLOAT_EQ(pos.position.y, 0.0f);
  EXPECT_FLOAT_EQ(pos.previous_position.x, 0.0f);
  EXPECT_FLOAT_EQ(pos.previous_position.y, 0.0f);

  client::ecs::VelocityComponent vel{};
  EXPECT_FLOAT_EQ(vel.velocity.x, 0.0f);
  EXPECT_FLOAT_EQ(vel.velocity.y, 0.0f);

  client::ecs::SpriteComponent sprite{};
  EXPECT_TRUE(sprite.texture_id.empty());
  EXPECT_TRUE(sprite.visible);
  EXPECT_FALSE(sprite.flip_x);
  EXPECT_FALSE(sprite.flip_y);

  client::ecs::RenderLayerComponent layer{};
  EXPECT_EQ(layer.layer, 0);
  EXPECT_FLOAT_EQ(layer.depth, 0.0f);

  client::ecs::HealthComponent health{};
  EXPECT_EQ(health.current, 0u);
  EXPECT_EQ(health.max, 0u);
  EXPECT_FALSE(health.alive());
}

TEST(ClientComponentsTest, ConstructorsInitializeValues) {
  client::ecs::NetworkedEntityComponent net{42u, 7u, 99u};
  EXPECT_EQ(net.network_id, 42u);
  EXPECT_EQ(net.type_code, 7u);
  EXPECT_EQ(net.last_snapshot, 99u);

  client::ecs::PositionComponent pos{5.0f, -2.0f};
  EXPECT_FLOAT_EQ(pos.position.x, 5.0f);
  EXPECT_FLOAT_EQ(pos.position.y, -2.0f);
  EXPECT_FLOAT_EQ(pos.previous_position.x, 5.0f);
  EXPECT_FLOAT_EQ(pos.previous_position.y, -2.0f);

  client::ecs::VelocityComponent vel{1.5f, -3.0f};
  EXPECT_FLOAT_EQ(vel.velocity.x, 1.5f);
  EXPECT_FLOAT_EQ(vel.velocity.y, -3.0f);

  engine::math::RectF rect{1.0f, 2.0f, 3.0f, 4.0f};
  client::ecs::SpriteComponent sprite{"asset", rect};
  EXPECT_EQ(sprite.texture_id, "asset");
  EXPECT_FLOAT_EQ(sprite.source_rect.top_left_x_, 1.0f);
  EXPECT_FLOAT_EQ(sprite.source_rect.top_left_y_, 2.0f);
  EXPECT_FLOAT_EQ(sprite.source_rect.width_, 3.0f);
  EXPECT_FLOAT_EQ(sprite.source_rect.height_, 4.0f);

  client::ecs::RenderLayerComponent layer{2, -1.0f};
  EXPECT_EQ(layer.layer, 2);
  EXPECT_FLOAT_EQ(layer.depth, -1.0f);

  client::ecs::HealthComponent health{5u, 10u};
  EXPECT_EQ(health.current, 5u);
  EXPECT_EQ(health.max, 10u);
  EXPECT_TRUE(health.alive());
}

TEST(ClientComponentsTest, HealthAliveReflectsZero) {
  client::ecs::HealthComponent health{0u, 10u};
  EXPECT_FALSE(health.alive());
  health.current = 1;
  EXPECT_TRUE(health.alive());
}

}  // namespace
