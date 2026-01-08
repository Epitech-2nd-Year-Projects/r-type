#include <gtest/gtest.h>

#include "engine/game_runtime.h"

TEST(GameRuntimeTest, Lifecycle) {
  engine::GameRuntime runtime;
  EXPECT_FALSE(runtime.Running());

  runtime.Start();
  EXPECT_TRUE(runtime.Running());

  runtime.Stop();
  EXPECT_FALSE(runtime.Running());
}

TEST(GameRuntimeTest, ModuleAccess) {
  engine::GameRuntime runtime;

  EXPECT_NO_THROW(runtime.Registry());
  EXPECT_NO_THROW(runtime.EventBus());

  auto& bus = runtime.EventBus();
  bool handled = false;
  bus.Subscribe<int>([&](const int& val) { handled = true; });
  bus.Publish(42);
  EXPECT_TRUE(handled);
}

TEST(GameRuntimeTest, MainThreadLoop) {
  engine::GameRuntime runtime;
  runtime.Start();

  int frames = 0;
  runtime.RunMainThread([&](engine::time::TimeDelta dt) {
    frames++;
    return frames < 5;
  });

  EXPECT_EQ(frames, 5);
  EXPECT_FALSE(runtime.Running());
}

TEST(GameRuntimeTest, LogicLoopIntegration) {
  engine::GameRuntime runtime;
  std::atomic<bool> event_processed{false};

  runtime.EventBus().Subscribe<int>(
      [&](const int& val) { event_processed = true; });

  runtime.Start();

  runtime.EventBus().PostTo(engine::util::ThreadId::kLogic, 100);

  auto start = std::chrono::steady_clock::now();
  while (!event_processed) {
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(1))
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  runtime.Stop();

  EXPECT_TRUE(event_processed);
}
