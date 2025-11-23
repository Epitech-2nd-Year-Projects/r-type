#include "../../include/engine/time/clock.h"

#include "clock_impl.h"

namespace engine::time {

Clock::Clock() : impl_(std::make_unique<ClockImpl>()) {}

Clock::~Clock() = default;

TimeDelta Clock::elapsed() const { return impl_->elapsed(); }

TimeDelta Clock::restart() {
  TimeDelta result = impl_->elapsed();
  impl_->reset();
  return result;
}

void Clock::reset() { impl_->reset(); }

}  // namespace engine::time