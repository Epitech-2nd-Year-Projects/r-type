/**
 * @file time_utils.h
 * @brief Thin helpers for monotonic time queries
 */

#ifndef CLIENT_TIME_UTILS_H_
#define CLIENT_TIME_UTILS_H_

#include <chrono>
#include <cstdint>

namespace client {

/**
 * @brief Current monotonic time in milliseconds
 */
inline std::uint64_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(duration_cast<milliseconds>(now).count());
}

}  // namespace client

#endif  // CLIENT_TIME_UTILS_H_
