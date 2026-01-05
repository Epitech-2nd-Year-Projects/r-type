#include "engine/profiling/network_profiler.h"

#include <algorithm>
#include <cmath>

namespace engine::profiling {

void NetworkProfiler::RecordLatency(float latency_ms) {
  current_latency_ms_ = latency_ms;
  latency_samples_.Push(latency_ms);
}

void NetworkProfiler::RecordPacketReceived() { ++packets_received_; }

void NetworkProfiler::RecordPacketDropped() { ++packets_dropped_; }

void NetworkProfiler::Reset() {
  latency_samples_.Clear();
  current_latency_ms_ = 0.0f;
  packets_received_ = 0;
  packets_dropped_ = 0;
}

NetworkStats NetworkProfiler::GetStats() const {
  NetworkStats stats;
  stats.latency_ms = current_latency_ms_;
  stats.min_latency_ms = latency_samples_.Min();
  stats.max_latency_ms = latency_samples_.Max();
  stats.packets_received = packets_received_;
  stats.packets_dropped = packets_dropped_;

  ComputeAvgAndJitter(stats.avg_latency_ms, stats.jitter_ms);

  std::uint64_t total = packets_received_ + packets_dropped_;
  if (total > 0) {
    stats.packet_loss_percent = static_cast<float>(packets_dropped_) /
                                static_cast<float>(total) * 100.0f;
  }

  return stats;
}

float NetworkProfiler::latency_ms() const { return current_latency_ms_; }

float NetworkProfiler::avg_latency_ms() const {
  return latency_samples_.Average();
}

float NetworkProfiler::jitter_ms() const {
  float avg = 0.0f, jitter = 0.0f;
  ComputeAvgAndJitter(avg, jitter);
  return jitter;
}

void NetworkProfiler::ComputeAvgAndJitter(float& avg, float& jitter) const {
  std::size_t count = latency_samples_.size();
  if (count == 0) {
    avg = 0.0f;
    jitter = 0.0f;
    return;
  }

  float sum = 0.0f;
  float sum_sq = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    float val = latency_samples_[i];
    sum += val;
    sum_sq += val * val;
  }

  avg = sum / static_cast<float>(count);

  if (count < 2) {
    jitter = 0.0f;
    return;
  }

  float variance = (sum_sq / static_cast<float>(count)) - (avg * avg);
  jitter = std::sqrt(std::max(variance, 0.0f));
}

}  // namespace engine::profiling
