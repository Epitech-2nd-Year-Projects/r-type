#include "engine/util/thread_safe_queue.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace engine::util;

TEST(ThreadSafeQueueTest, PushAndPop) {
  ThreadSafeQueue<int> queue;

  queue.Push(1);
  queue.Push(2);
  queue.Push(3);

  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(queue.Size(), 3);

  int val;
  EXPECT_TRUE(queue.TryPop(val));
  EXPECT_EQ(val, 1);

  auto opt = queue.TryPop();
  ASSERT_TRUE(opt.has_value());
  EXPECT_EQ(*opt, 2);

  queue.WaitAndPop(val);
  EXPECT_EQ(val, 3);

  EXPECT_TRUE(queue.Empty());
}

TEST(ThreadSafeQueueTest, TryPopEmpty) {
  ThreadSafeQueue<int> queue;
  int val;
  EXPECT_FALSE(queue.TryPop(val));
  EXPECT_FALSE(queue.TryPop().has_value());
}

TEST(ThreadSafeQueueTest, WaitAndPopBlocks) {
  ThreadSafeQueue<int> queue;
  std::atomic<bool> pushed{false};

  std::thread producer([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Push(42);
    pushed = true;
  });

  int val = 0;
  auto start = std::chrono::steady_clock::now();
  queue.WaitAndPop(val);
  auto end = std::chrono::steady_clock::now();

  EXPECT_TRUE(pushed);
  EXPECT_EQ(val, 42);
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_GE(duration.count(), 40);

  producer.join();
}

TEST(ThreadSafeQueueTest, WaitAndPopForTimeout) {
  ThreadSafeQueue<int> queue;
  int val;
  bool result = queue.WaitAndPopFor(val, std::chrono::milliseconds(10));
  EXPECT_FALSE(result);
}

TEST(ThreadSafeQueueTest, WaitAndPopForSuccess) {
  ThreadSafeQueue<int> queue;

  std::thread producer([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    queue.Push(100);
  });

  int val;
  bool result = queue.WaitAndPopFor(val, std::chrono::milliseconds(100));
  EXPECT_TRUE(result);
  EXPECT_EQ(val, 100);

  producer.join();
}

TEST(ThreadSafeQueueTest, MoveOnlyTypes) {
  ThreadSafeQueue<std::unique_ptr<int>> queue;

  queue.Push(std::make_unique<int>(123));

  std::unique_ptr<int> ptr;
  EXPECT_TRUE(queue.TryPop(ptr));
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 123);
}

TEST(ThreadSafeQueueTest, ConcurrentAccess) {
  ThreadSafeQueue<int> queue;
  const int kCount = 1000;

  std::thread producer([&] {
    for (int i = 0; i < kCount; ++i) {
      queue.Push(i);
    }
  });

  std::thread consumer([&] {
    for (int i = 0; i < kCount; ++i) {
      int val;
      queue.WaitAndPop(val);
    }
  });

  producer.join();
  consumer.join();

  EXPECT_TRUE(queue.Empty());
}
