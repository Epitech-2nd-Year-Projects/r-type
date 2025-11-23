#ifndef ENGINE_TIME_INTERNAL_CLOCK_IMPL_H_
#define ENGINE_TIME_INTERNAL_CLOCK_IMPL_H_

#include "../../include/engine/time/time_delta.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace engine::time {

/**
 * @class ClockImpl
 * @brief Platform-specific clock implementation (PRIVATE)
 *
 * This is an internal implementation detail. Users should not
 * interact with this directly - use Clock instead.
 *
 * Abstracts OS-specific timing:
 * - Windows: QueryPerformanceCounter
 * - POSIX: clock_gettime(CLOCK_MONOTONIC)
 */
class ClockImpl {
 public:
  /**
   * @brief Initialize clock (starts measuring from now)
   */
  ClockImpl();

  /**
   * @brief Get elapsed time since creation or last reset()
   */
  TimeDelta elapsed() const;

  /**
   * @brief Reset clock to zero
   */
  void reset();

 private:
#ifdef _WIN32
  LARGE_INTEGER frequency_;
  LARGE_INTEGER start_time_;
#else
  timespec start_time_;
#endif
};

}  // namespace engine::time

#endif  // ENGINE_TIME_INTERNAL_CLOCK_IMPL_H_