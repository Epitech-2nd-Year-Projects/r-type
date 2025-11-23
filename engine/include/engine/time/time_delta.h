#ifndef ENGINE_TIME_TIME_DELTA_H_
#define ENGINE_TIME_TIME_DELTA_H_

#include <cstdint>
#include <iostream>

namespace engine::time {

/**
 * @class TimeDelta
 * @brief Type-safe representation of a time duration
 *
 * Stores time internally as microseconds (int64_t) for precision and
 * fast arithmetic without conversion overhead.
 *
 * @section usage Usage
 * @code
 * TimeDelta frame_time = TimeDelta::from_seconds(0.016f);  // ~60 FPS
 * TimeDelta delay = TimeDelta::from_milliseconds(500);
 *
 * float seconds = frame_time.as_seconds();
 * float millis = frame_time.as_milliseconds();
 *
 * TimeDelta total = frame_time + delay;
 * TimeDelta half = frame_time / 2.0f;
 *
 * if (frame_time > TimeDelta::from_milliseconds(20)) {
 *   LOG_WARNING("Frame took too long");
 * }
 * @endcode
 */
class TimeDelta {
 public:
  /**
   * @brief Create TimeDelta from seconds
   */
  static TimeDelta from_seconds(float seconds) noexcept;

  /**
   * @brief Create TimeDelta from milliseconds
   */
  static TimeDelta from_milliseconds(float milliseconds) noexcept;

  /**
   * @brief Create TimeDelta from microseconds
   */
  static TimeDelta from_microseconds(int64_t microseconds) noexcept;

  /**
   * @brief Create a zero duration
   */
  static TimeDelta zero() noexcept;

  /**
   * @brief Get duration as fractional seconds
   */
  float as_seconds() const noexcept;

  /**
   * @brief Get duration as fractional milliseconds
   */
  float as_milliseconds() const noexcept;

  /**
   * @brief Get duration as integer microseconds
   */
  int64_t as_microseconds() const noexcept;

  TimeDelta operator+(const TimeDelta& other) const noexcept;
  TimeDelta operator-(const TimeDelta& other) const noexcept;
  TimeDelta operator*(float scalar) const noexcept;
  friend TimeDelta operator*(float scalar, const TimeDelta& delta) noexcept;
  TimeDelta operator/(float scalar) const;
  float operator/(const TimeDelta& other) const;
  TimeDelta& operator+=(const TimeDelta& other) noexcept;
  TimeDelta& operator-=(const TimeDelta& other) noexcept;
  TimeDelta& operator*=(float scalar) noexcept;
  TimeDelta& operator/=(float scalar);

  bool operator==(const TimeDelta& other) const noexcept;
  bool operator!=(const TimeDelta& other) const noexcept;
  bool operator<(const TimeDelta& other) const noexcept;
  bool operator<=(const TimeDelta& other) const noexcept;
  bool operator>(const TimeDelta& other) const noexcept;
  bool operator>=(const TimeDelta& other) const noexcept;

 private:
  int64_t microseconds_;

  explicit TimeDelta(int64_t microseconds) noexcept
      : microseconds_(microseconds) {}

  friend class Clock;

  static constexpr int64_t kMicrosecondsPerSecond = 1'000'000;
  static constexpr int64_t kMicrosecondsPerMillisecond = 1'000;
};

}  // namespace engine::time

#endif  // ENGINE_TIME_TIME_DELTA_H_