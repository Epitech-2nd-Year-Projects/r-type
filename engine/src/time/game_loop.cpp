#include "../../include/engine/time/game_loop.h"

namespace engine::time {

VariableTimestepLoop::VariableTimestepLoop(float target_fps)
    : frame_timer_(target_fps) {}

void VariableTimestepLoop::run(std::function<bool(TimeDelta)> callback) {
  while (true) {
    if (!tick(callback)) break;
  }
}

bool VariableTimestepLoop::tick(std::function<bool(TimeDelta)> callback) {
  TimeDelta dt = frame_timer_.tick();
  return callback(dt);
}

FixedTimestepLoop::FixedTimestepLoop(TimeDelta fixed_timestep, float target_fps)
    : fixed_timestep_(fixed_timestep),
      frame_timer_(target_fps),
      accumulated_time_(TimeDelta::zero()) {}

void FixedTimestepLoop::run(
    std::function<bool(TimeDelta, TimeDelta)> callback) {
  while (true) {
    if (!tick(callback)) break;
  }
}

bool FixedTimestepLoop::tick(
    std::function<bool(TimeDelta, TimeDelta)> callback) {
  TimeDelta frame_dt = frame_timer_.tick();
  accumulated_time_ += frame_dt;
  constexpr int max_iterations = 10;
  int iterations = 0;
  const TimeDelta max_accumulated = TimeDelta::from_milliseconds(100.0f);

  if (accumulated_time_ > max_accumulated) {
    accumulated_time_ = max_accumulated;
  }
  while (accumulated_time_ >= fixed_timestep_ && iterations < max_iterations) {
    accumulated_time_ -= fixed_timestep_;
    if (!callback(fixed_timestep_, frame_dt)) return false;
    iterations++;
  }
  return true;
}

void FixedTimestepLoop::set_fixed_timestep(TimeDelta timestep) {
  fixed_timestep_ = timestep;
}

}  // namespace engine::time