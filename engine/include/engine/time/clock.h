#ifndef ENGINE_TIME_CLOCK_H_
#define ENGINE_TIME_CLOCK_H_

#include <memory>

#include "time_delta.h"

namespace engine::time {

class ClockImpl;

/**
 * @class Clock
 * @brief High-resolution timer for measuring elapsed time
 *
 * Uses OS-specific high-resolution timers:
 * - Windows: QueryPerformanceCounter
 * - POSIX: clock_gettime(CLOCK_MONOTONIC)
 *
 * @note
 * - Uses smart pointers (std::unique_ptr) for memory safety
 * - Monotonic (always moves forward)
 * - Microsecond precision
 * - Not suitable for absolute timestamps
 *
 * @section usage Usage
 * @code
 * // Measure elapsed time
 * Clock clock;
 * // ... do work ...
 * TimeDelta elapsed = clock.elapsed();
 * std::cout << "Work took: " << elapsed << "\n";
 *
 * // Frame timing (typical pattern)
 * Clock frame_clock;
 * while (game_running) {
 *   TimeDelta dt = frame_clock.restart();
 *   update(dt);
 *   render();
 * }
 * @endcode
 */
class Clock {
 public:
  /**
   * @brief Create clock and start measuring from now
   *
   * Initializes the clock immediately. Time 0 is the moment
   * of construction.
   */
  Clock();

  /**
   * @brief Destructor (automatic cleanup via unique_ptr)
   */
  ~Clock();

  /**
   * @brief Get elapsed time since creation or last restart()
   * @return TimeDelta representing elapsed time
   *
   * Calling this multiple times returns different values
   * (the clock is continuously running).
   *
   * @example
   * @code
   * Clock clock;
   * // ... wait 100ms ...
   * TimeDelta elapsed1 = clock.elapsed();  // ~100ms
   * // ... wait 50ms more ...
   * TimeDelta elapsed2 = clock.elapsed();  // ~150ms
   * @endcode
   */
  TimeDelta elapsed() const;

  /**
   * @brief Get elapsed time and restart clock
   * @return TimeDelta representing elapsed time since creation or last
   * restart()
   *
   * After this call, elapsed() will return values relative to the call time.
   * This is the typical pattern for frame delta timing.
   *
   * @example
   * @code
   * Clock frame_timer;
   * while (game_running) {
   *   TimeDelta dt = frame_timer.restart();
   *   update_game(dt);
   * }
   * @endcode
   */
  TimeDelta restart();

  /**
   * @brief Reset clock to zero
   *
   * Sets elapsed time to 0 and begins counting again.
   * Less common than restart() but useful for explicit resets.
   *
   * @example
   * @code
   * if (condition) {
   *   clock.reset();  // Discard previous timing, start fresh
   * }
   * @endcode
   */
  void reset();

 private:
  // Unique pointer to platform-specific implementation
  std::unique_ptr<ClockImpl> impl_;

  // Prevent copying
  Clock(const Clock&) = delete;
  Clock& operator=(const Clock&) = delete;

  // Allow move semantics
  Clock(Clock&&) noexcept = default;
  Clock& operator=(Clock&&) noexcept = default;
};

}  // namespace engine::time

#endif  // ENGINE_TIME_CLOCK_H_