#include "../../include/engine/time/frame_timer.h"

namespace engine::time {

FrameTimer::FrameTimer(float target_fps)
    : delta_time_(TimeDelta::zero()),
      target_frame_time_(TimeDelta::from_seconds(1.0f / target_fps)) {
  clock_.restart();
}

TimeDelta FrameTimer::tick() {
  delta_time_ = clock_.restart();
  return delta_time_;
}

TimeDelta FrameTimer::delta_time() const { return delta_time_; }

void FrameTimer::reset() {
  delta_time_ = TimeDelta::zero();
  clock_.reset();
}

void FrameTimer::set_target_fps(float fps) {
  if (fps <= 0.0f)
    throw std::invalid_argument("FPS must be > 0");
  target_frame_time_ = TimeDelta::from_seconds(1.0f / fps);
}

}  // namespace engine::time