#ifndef ENGINE_PROFILING_FRAME_PROFILER_H_
#define ENGINE_PROFILING_FRAME_PROFILER_H_

#include <atomic>

#include "engine/profiling/ring_buffer.h"
#include "engine/time/time_delta.h"

namespace engine::profiling {

struct FrameStats {
  float current_fps{0.0f};
  float frame_time_ms{0.0f};
  float avg_frame_time_ms{0.0f};
  float min_frame_time_ms{0.0f};
  float max_frame_time_ms{0.0f};
};

class FrameProfiler {
 public:
  static constexpr std::size_t kSampleCount = 256;

  void RecordFrame(time::TimeDelta dt);
  void Reset();

  FrameStats GetStats() const;
  float current_fps() const;
  float frame_time_ms() const;
  float avg_frame_time_ms() const;
  float min_frame_time_ms() const;
  float max_frame_time_ms() const;

  const RingBuffer<float, kSampleCount>& frame_times() const {
    return frame_times_;
  }

 private:
  RingBuffer<float, kSampleCount> frame_times_;
  std::atomic<float> current_fps_{0.0f};
  std::atomic<float> current_frame_time_ms_{0.0f};
};

}  // namespace engine::profiling

#endif  // ENGINE_PROFILING_FRAME_PROFILER_H_
