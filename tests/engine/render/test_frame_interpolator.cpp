#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "engine/render/frame_interpolator.h"
#include "engine/render/snapshot_buffer.h"

using namespace engine::render;

TEST(FrameInterpolatorTest, InterpolatesPositions) {
  SnapshotBuffer buffer;
  FrameInterpolator interpolator(buffer);

  RenderSnapshot s1;
  s1.tick = 1;
  s1.valid = true;
  s1.timestamp_ns = 1000;
  SpriteData d1;
  d1.entity_id = 1;
  d1.position = {0.0f, 0.0f};
  d1.visible = true;
  s1.sprites.push_back(d1);
  buffer.Produce(std::move(s1));

  RenderSnapshot s2;
  s2.tick = 2;
  s2.valid = true;
  s2.timestamp_ns = 2000;
  SpriteData d2;
  d2.entity_id = 1;
  d2.position = {100.0f, 100.0f};
  d2.visible = true;
  s2.sprites.push_back(d2);
  buffer.Produce(std::move(s2));

  auto sprites = interpolator.Interpolate(1500);

  ASSERT_EQ(sprites.size(), 1);
  EXPECT_NEAR(sprites[0].position.x, 50.0f, 0.001f);
  EXPECT_NEAR(sprites[0].position.y, 50.0f, 0.001f);
}

TEST(FrameInterpolatorTest, RecursionHandlesSpawn) {
  SnapshotBuffer buffer;
  FrameInterpolator interpolator(buffer);

  RenderSnapshot s1;
  s1.tick = 1;
  s1.valid = true;
  s1.timestamp_ns = 1000;
  buffer.Produce(std::move(s1));

  RenderSnapshot s2;
  s2.tick = 2;
  s2.valid = true;
  s2.timestamp_ns = 2000;
  SpriteData d2;
  d2.entity_id = 99;
  d2.position = {500.0f, 500.0f};
  d2.visible = true;
  s2.sprites.push_back(d2);
  buffer.Produce(std::move(s2));

  auto sprites = interpolator.Interpolate(1500);

  ASSERT_EQ(sprites.size(), 1);
  EXPECT_FLOAT_EQ(sprites[0].position.x, 500.0f);
  EXPECT_FLOAT_EQ(sprites[0].position.y, 500.0f);
}

TEST(FrameInterpolatorTest, ClampsAlpha) {
  SnapshotBuffer buffer;
  FrameInterpolator interpolator(buffer);

  RenderSnapshot s1;
  s1.valid = true;
  s1.timestamp_ns = 1000;
  buffer.Produce(std::move(s1));

  RenderSnapshot s2;
  s2.valid = true;
  s2.timestamp_ns = 2000;
  SpriteData d;
  d.entity_id = 1;
  d.position = {10.0f, 0.0f};
  d.visible = true;
  s2.sprites.push_back(d);
  buffer.Produce(std::move(s2));

  auto sprites = interpolator.Interpolate(3000);

  ASSERT_EQ(sprites.size(), 1);
  EXPECT_FLOAT_EQ(sprites[0].position.x, 10.0f);
}
