#include "../../include/engine/time/time_delta.h"


namespace engine::time {

TimeDelta TimeDelta::from_seconds(float seconds) noexcept {
  return TimeDelta(static_cast<int64_t>(seconds * kMicrosecondsPerSecond));
}

TimeDelta TimeDelta::from_milliseconds(float milliseconds) noexcept {
  return TimeDelta(
      static_cast<int64_t>(milliseconds * kMicrosecondsPerMillisecond));
}

TimeDelta TimeDelta::from_microseconds(int64_t microseconds) noexcept {
  return TimeDelta(microseconds);
}

TimeDelta TimeDelta::zero() noexcept { return TimeDelta(0); }

float TimeDelta::as_seconds() const noexcept {
  return static_cast<float>(microseconds_) /
         static_cast<float>(kMicrosecondsPerSecond);
}

float TimeDelta::as_milliseconds() const noexcept {
  return static_cast<float>(microseconds_) /
         static_cast<float>(kMicrosecondsPerMillisecond);
}
int64_t TimeDelta::as_microseconds() const noexcept { return microseconds_; }

TimeDelta TimeDelta::operator+(const TimeDelta& other) const noexcept {
  return TimeDelta(microseconds_ + other.microseconds_);
}

TimeDelta TimeDelta::operator-(const TimeDelta& other) const noexcept {
  return TimeDelta(microseconds_ - other.microseconds_);
}

TimeDelta TimeDelta::operator*(float scalar) const noexcept {
  return TimeDelta(static_cast<int64_t>(microseconds_ * scalar));
}

TimeDelta operator*(float scalar, const TimeDelta& delta) noexcept {
  return delta * scalar;
}

TimeDelta TimeDelta::operator/(float scalar) const {
  if (scalar == 0.0f) {
    throw std::invalid_argument("Division by zero");
  }
  return TimeDelta(static_cast<int64_t>(microseconds_ / scalar));
}

float TimeDelta::operator/(const TimeDelta& other) const {
  if (other.microseconds_ == 0) {
    throw std::invalid_argument("Division by zero");
  }
  return static_cast<float>(microseconds_) / other.microseconds_;
}

TimeDelta& TimeDelta::operator+=(const TimeDelta& other) noexcept {
  microseconds_ += other.microseconds_;
  return *this;
}

TimeDelta& TimeDelta::operator-=(const TimeDelta& other) noexcept {
  microseconds_ -= other.microseconds_;
  return *this;
}

TimeDelta& TimeDelta::operator*=(float scalar) noexcept {
  microseconds_ = static_cast<int64_t>(microseconds_ * scalar);
  return *this;
}

TimeDelta& TimeDelta::operator/=(float scalar) {
  if (scalar == 0.0f) {
    throw std::invalid_argument("Division by zero");
  }
  microseconds_ = static_cast<int64_t>(microseconds_ / scalar);
  return *this;
}

bool TimeDelta::operator==(const TimeDelta& other) const noexcept {
  return microseconds_ == other.microseconds_;
}

bool TimeDelta::operator!=(const TimeDelta& other) const noexcept {
  return microseconds_ != other.microseconds_;
}

bool TimeDelta::operator<(const TimeDelta& other) const noexcept {
  return microseconds_ < other.microseconds_;
}

bool TimeDelta::operator<=(const TimeDelta& other) const noexcept {
  return microseconds_ <= other.microseconds_;
}

bool TimeDelta::operator>(const TimeDelta& other) const noexcept {
  return microseconds_ > other.microseconds_;
}

bool TimeDelta::operator>=(const TimeDelta& other) const noexcept {
  return microseconds_ >= other.microseconds_;
}

}  // namespace engine::time