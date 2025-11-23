#include "clock_impl.h"

namespace engine::time {

#ifdef _WIN32

ClockImpl::ClockImpl() {
  QueryPerformanceFrequency(&frequency_);
  QueryPerformanceCounter(&start_time_);
}

TimeDelta ClockImpl::elapsed() const {
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);

  int64_t counts = current.QuadPart - start_time_.QuadPart;
  int64_t microseconds = (counts * 1'000'000) / frequency_.QuadPart;

  return TimeDelta::from_microseconds(microseconds);
}

void ClockImpl::reset() { QueryPerformanceCounter(&start_time_); }

#else

ClockImpl::ClockImpl() { clock_gettime(CLOCK_MONOTONIC, &start_time_); }

TimeDelta ClockImpl::elapsed() const {
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  int64_t sec_diff = current.tv_sec - start_time_.tv_sec;
  int64_t nsec_diff = current.tv_nsec - start_time_.tv_nsec;

  int64_t total_nanoseconds = sec_diff * 1'000'000'000 + nsec_diff;
  int64_t total_microseconds = total_nanoseconds / 1'000;

  return TimeDelta::from_microseconds(total_microseconds);
}

void ClockImpl::reset() { clock_gettime(CLOCK_MONOTONIC, &start_time_); }

#endif

}  // namespace engine::time