#include <gtest/gtest.h>

#include <thread>
#include <variant>

#include "engine/profiler/profiler.h"

TEST(ProfilerTest, SetAndGetMetrics) {
  auto& profiler = engine::profiler::Profiler::Get();

  profiler.SetMetric("test_int", 42);
  profiler.SetMetric("test_float", 3.14f);
  profiler.SetMetric("test_string", std::string("hello"));

  auto metrics = profiler.GetMetrics();

  ASSERT_TRUE(metrics.count("test_int"));
  ASSERT_TRUE(std::holds_alternative<int>(metrics.at("test_int")));
  EXPECT_EQ(std::get<int>(metrics.at("test_int")), 42);

  ASSERT_TRUE(metrics.count("test_float"));
  ASSERT_TRUE(std::holds_alternative<float>(metrics.at("test_float")));
  EXPECT_NEAR(std::get<float>(metrics.at("test_float")), 3.14f, 0.001f);

  ASSERT_TRUE(metrics.count("test_string"));
  ASSERT_TRUE(std::holds_alternative<std::string>(metrics.at("test_string")));
  EXPECT_EQ(std::get<std::string>(metrics.at("test_string")), "hello");
}

TEST(ProfilerTest, RecordSampleAddsToHistory) {
  auto& profiler = engine::profiler::Profiler::Get();
  std::string metric_name = "test_history";

  profiler.RecordSample(metric_name, 10.0f);
  profiler.RecordSample(metric_name, 20.0f);

  auto histories = profiler.GetHistories();
  ASSERT_TRUE(histories.count(metric_name));

  const auto& history = histories.at(metric_name);

  ASSERT_GE(history.values.size(), 2);
  size_t size = history.values.size();
  EXPECT_EQ(history.values[size - 2], 10.0f);
  EXPECT_EQ(history.values[size - 1], 20.0f);

  EXPECT_LE(history.min_value, 10.0f);
  EXPECT_GE(history.max_value, 20.0f);
}

TEST(ProfilerTest, MetricHistoryCap) {
  engine::profiler::MetricHistory history;
  history.max_samples = 5;

  for (int i = 0; i < 10; ++i) {
    history.Add(static_cast<float>(i));
  }

  EXPECT_EQ(history.values.size(), 5);
  EXPECT_EQ(history.values.front(), 5.0f);
  EXPECT_EQ(history.values.back(), 9.0f);

  EXPECT_LE(history.min_value, 5.0f);
  EXPECT_GE(history.max_value, 9.0f);
}

TEST(ProfilerTest, ScopedTimerRecordsDuration) {
  auto& profiler = engine::profiler::Profiler::Get();
  std::string timer_name = "test_timer";

  {
    engine::profiler::ScopedTimer timer(timer_name);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  auto histories = profiler.GetHistories();
  ASSERT_TRUE(histories.count(timer_name));
  EXPECT_FALSE(histories.at(timer_name).values.empty());
  EXPECT_GT(histories.at(timer_name).values.back(), 0.0f);
}

TEST(ProfilerTest, OverwriteMetricType) {
  auto& profiler = engine::profiler::Profiler::Get();
  std::string key = "test_overwrite";

  profiler.SetMetric(key, 100);
  ASSERT_TRUE(std::holds_alternative<int>(profiler.GetMetrics().at(key)));

  profiler.SetMetric(key, std::string("text"));
  ASSERT_TRUE(
      std::holds_alternative<std::string>(profiler.GetMetrics().at(key)));
  EXPECT_EQ(std::get<std::string>(profiler.GetMetrics().at(key)), "text");
}
