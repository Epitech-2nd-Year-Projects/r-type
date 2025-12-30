#include "engine/profiling/network_profiler.h"

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
  stats.avg_latency_ms = avg_latency_ms();
  stats.min_latency_ms = latency_samples_.Min();
  stats.max_latency_ms = latency_samples_.Max();
  stats.jitter_ms = jitter_ms();
  stats.packets_received = packets_received_;
  stats.packets_dropped = packets_dropped_;

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

float NetworkProfiler::jitter_ms() const { return ComputeJitter(); }

float NetworkProfiler::ComputeJitter() const {
  if (latency_samples_.size() < 2) return 0.0f;

  float avg = latency_samples_.Average();
  float variance = 0.0f;
  std::size_t count = latency_samples_.size();

  for (std::size_t i = 0; i < count; ++i) {
    float diff = latency_samples_[i] - avg;
    variance += diff * diff;
  }

  variance /= static_cast<float>(count);
  return std::sqrt(variance);
}

}  // namespace engine::profiling
