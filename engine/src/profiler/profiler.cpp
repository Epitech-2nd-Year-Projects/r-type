#include "engine/profiler/profiler.h"

#include <algorithm>

namespace engine::profiler {

void MetricHistory::Add(float value) {
  values.push_back(value);
  if (values.size() > max_samples) {
    values.pop_front();
  }

  if (values.empty()) {
    min_value = 0.0f;
    max_value = 0.0f;
    return;
  }

  min_value = values.front();
  max_value = values.front();
  for (float v : values) {
    min_value = std::min(min_value, v);
    max_value = std::max(max_value, v);
  }
}

Profiler& Profiler::Get() {
  static Profiler instance;
  return instance;
}

void Profiler::SetMetric(const std::string& name, MetricValue value) {
  std::lock_guard<std::mutex> lock(mutex_);
  metrics_[name] = value;
}

void Profiler::RecordSample(const std::string& name, float value) {
  std::lock_guard<std::mutex> lock(mutex_);
  histories_[name].Add(value);
  metrics_[name] = value;
}

std::unordered_map<std::string, MetricValue>& Profiler::GetMetrics() {
  return metrics_;
}

std::unordered_map<std::string, MetricHistory>& Profiler::GetHistories() {
  return histories_;
}

ScopedTimer::ScopedTimer(const std::string& name)
    : name_(name), start_time_(std::chrono::steady_clock::now()) {}

ScopedTimer::~ScopedTimer() {
  auto end_time = std::chrono::steady_clock::now();
  std::chrono::duration<float, std::milli> duration = end_time - start_time_;
  Profiler::Get().RecordSample(name_, duration.count());
}

}  // namespace engine::profiler
