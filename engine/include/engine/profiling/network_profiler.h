#ifndef ENGINE_PROFILING_NETWORK_PROFILER_H_
#define ENGINE_PROFILING_NETWORK_PROFILER_H_

#include <cstddef>
#include <cstdint>

#include "engine/profiling/ring_buffer.h"

namespace engine::profiling {

struct NetworkStats {
  float latency_ms{0.0f};
  float avg_latency_ms{0.0f};
  float min_latency_ms{0.0f};
  float max_latency_ms{0.0f};
  float jitter_ms{0.0f};
  std::uint64_t packets_received{0};
  std::uint64_t packets_dropped{0};
  float packet_loss_percent{0.0f};
};

class NetworkProfiler {
 public:
  static constexpr std::size_t kSampleCount = 128;

  void RecordLatency(float latency_ms);
  void RecordPacketReceived();
  void RecordPacketDropped();
  void Reset();

  NetworkStats GetStats() const;
  float latency_ms() const;
  float avg_latency_ms() const;
  float jitter_ms() const;
  std::uint64_t packets_dropped() const { return packets_dropped_; }

  const RingBuffer<float, kSampleCount>& latency_samples() const {
    return latency_samples_;
  }

 private:
  float ComputeJitter() const;

  RingBuffer<float, kSampleCount> latency_samples_;
  float current_latency_ms_{0.0f};
  std::uint64_t packets_received_{0};
  std::uint64_t packets_dropped_{0};
};

}  // namespace engine::profiling

#endif  // ENGINE_PROFILING_NETWORK_PROFILER_H_
