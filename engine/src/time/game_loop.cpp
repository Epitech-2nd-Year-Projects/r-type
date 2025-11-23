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

  while (accumulated_time_ >= fixed_timestep_) {
    accumulated_time_ -= fixed_timestep_;
    if (!callback(fixed_timestep_, frame_dt)) return false;
  }
  return true;
}

void FixedTimestepLoop::set_fixed_timestep(TimeDelta timestep) {
  fixed_timestep_ = timestep;
}

}  // namespace engine::time