#include "engine/profiling/frame_profiler.h"

namespace engine::profiling {

void FrameProfiler::RecordFrame(time::TimeDelta dt) {
  float seconds = dt.as_seconds();
  current_frame_time_ms_ = seconds * 1000.0f;
  current_fps_ = seconds > 0.0f ? 1.0f / seconds : 0.0f;
  frame_times_.Push(current_frame_time_ms_);
}

void FrameProfiler::Reset() {
  frame_times_.Clear();
  current_fps_ = 0.0f;
  current_frame_time_ms_ = 0.0f;
}

FrameStats FrameProfiler::GetStats() const {
  return {
      .current_fps = current_fps_,
      .frame_time_ms = current_frame_time_ms_,
      .avg_frame_time_ms = avg_frame_time_ms(),
      .min_frame_time_ms = min_frame_time_ms(),
      .max_frame_time_ms = max_frame_time_ms(),
  };
}

float FrameProfiler::current_fps() const { return current_fps_; }

float FrameProfiler::frame_time_ms() const { return current_frame_time_ms_; }

float FrameProfiler::avg_frame_time_ms() const {
  return frame_times_.Average();
}

float FrameProfiler::min_frame_time_ms() const { return frame_times_.Min(); }

float FrameProfiler::max_frame_time_ms() const { return frame_times_.Max(); }

}  // namespace engine::profiling
