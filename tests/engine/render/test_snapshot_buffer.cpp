#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "engine/render/render_snapshot.h"
#include "engine/render/snapshot_buffer.h"

using namespace engine::render;

TEST(SnapshotBufferTest, ProducesAndConsumes) {
  SnapshotBuffer buffer;
  RenderSnapshot s1;
  s1.tick = 1;

  buffer.Produce(std::move(s1));

  const auto& consumed = buffer.Consume();
  EXPECT_EQ(consumed.tick, 1);
}

TEST(SnapshotBufferTest, ConcurrentProducerConsumer) {
  SnapshotBuffer buffer;
  std::atomic<bool> running{true};

  std::thread producer([&]() {
    uint32_t tick = 1;
    while (running) {
      RenderSnapshot s;
      s.tick = tick++;
      s.valid = true;
      buffer.Produce(std::move(s));
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  std::thread consumer([&]() {
    uint32_t last_tick = 0;
    while (running) {
      const auto& s = buffer.Consume();
      if (s.valid) {
        // We might skip ticks, but we should strictly increase
        // (unless we read the same snapshot multiple times if producer is slow)
        EXPECT_GE(s.tick, last_tick);
        last_tick = s.tick;
      }
      std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  running = false;
  producer.join();
  consumer.join();
}

TEST(SnapshotBufferTest, InterpolationPair) {
  SnapshotBuffer buffer;

  RenderSnapshot s1;
  s1.tick = 10;
  RenderSnapshot s2;
  s2.tick = 20;

  buffer.Produce(std::move(s1));
  buffer.Produce(std::move(s2));

  auto pair = buffer.GetInterpolationPair();
  SnapshotBuffer buffer_flow;

  RenderSnapshot snap1;
  snap1.tick = 100;
  buffer_flow.Produce(std::move(snap1));

  auto pair1 = buffer_flow.GetInterpolationPair();
  EXPECT_EQ(pair1.second.tick, 100);

  RenderSnapshot snap2;
  snap2.tick = 200;
  buffer_flow.Produce(std::move(snap2));

  auto pair2 = buffer_flow.GetInterpolationPair();
  EXPECT_EQ(pair2.first.tick, 100);
  EXPECT_EQ(pair2.second.tick, 200);
}
