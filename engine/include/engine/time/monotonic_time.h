/**
 * @file monotonic_time.h
 * @brief Monotonic timestamp helpers in milliseconds
 */

#ifndef ENGINE_TIME_MONOTONIC_TIME_H_
#define ENGINE_TIME_MONOTONIC_TIME_H_

#include <chrono>
#include <cstdint>

namespace engine::time {

/**
 * @brief Current monotonic time in milliseconds
 */
inline std::uint64_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(duration_cast<milliseconds>(now).count());
}

}  // namespace engine::time

#endif  // ENGINE_TIME_MONOTONIC_TIME_H_
