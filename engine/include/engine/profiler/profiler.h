#ifndef ENGINE_PROFILER_PROFILER_H_
#define ENGINE_PROFILER_PROFILER_H_

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "engine/time.h"
#include "engine/time/monotonic_time.h"

namespace engine::profiler {

using MetricValue = std::variant<float, int, std::string>;

struct MetricHistory {
  std::deque<float> values;
  size_t max_samples{120};
  float min_value{0.0f};
  float max_value{0.0f};

  void Add(float value);
};

class Profiler {
 public:
  static Profiler& Get();

  void SetMetric(const std::string& name, MetricValue value);
  void RecordSample(const std::string& name, float value);

  std::unordered_map<std::string, MetricValue> GetMetrics();
  std::unordered_map<std::string, MetricHistory> GetHistories();

 private:
  Profiler() = default;
  ~Profiler() = default;

  std::mutex mutex_;
  std::unordered_map<std::string, MetricValue> metrics_;
  std::unordered_map<std::string, MetricHistory> histories_;
};

class ScopedTimer {
 public:
  ScopedTimer(std::string_view name);
  ~ScopedTimer();

 private:
  std::string_view name_;
  std::chrono::steady_clock::time_point start_time_;
};

}  // namespace engine::profiler

#endif  // ENGINE_PROFILER_PROFILER_H_
