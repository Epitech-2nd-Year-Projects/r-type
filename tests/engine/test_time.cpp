#include <gtest/gtest.h>

#include <thread>

#include "engine/time/clock.h"
#include "engine/time/time_delta.h"

TEST(TimeDeltaTest, Conversions) {
  auto dt = engine::time::TimeDelta::from_seconds(1.0f);

  EXPECT_FLOAT_EQ(dt.as_seconds(), 1.0f);
  EXPECT_FLOAT_EQ(dt.as_milliseconds(), 1000.0f);

  auto dt_ms = engine::time::TimeDelta::from_milliseconds(500.0f);
  EXPECT_FLOAT_EQ(dt_ms.as_seconds(), 0.5f);
}

TEST(TimeDeltaTest, Operations) {
  auto dt1 = engine::time::TimeDelta::from_seconds(1.0f);
  auto dt2 = engine::time::TimeDelta::from_seconds(2.0f);

  auto sum = dt1 + dt2;
  EXPECT_FLOAT_EQ(sum.as_seconds(), 3.0f);

  auto diff = dt2 - dt1;
  EXPECT_FLOAT_EQ(diff.as_seconds(), 1.0f);

  auto scaled = dt1 * 2.0f;
  EXPECT_FLOAT_EQ(scaled.as_seconds(), 2.0f);
}

TEST(ClockTest, MeasureElapsedTime) {
  engine::time::Clock clock;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto elapsed = clock.elapsed();
  EXPECT_GT(elapsed.as_milliseconds(), 0.0f);
}

TEST(ClockTest, RestartResetsZero) {
  engine::time::Clock clock;

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  auto first = clock.restart();
  EXPECT_GT(first.as_milliseconds(), 0.0f);

  auto second = clock.elapsed();
  EXPECT_LT(second.as_milliseconds(), 10.0f);
}
