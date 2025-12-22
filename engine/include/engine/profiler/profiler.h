/**
 * @file profiler.h
 * @brief Performance and state profiling system
 * @version 1.0.0
 *
 * @details
 * The profiler system provides a lightweight thread-safe mechanism to track
 * runtime metrics and performance data. It supports immediate value metrics
 * and historical data tracking which can be visualized in the ProfilerOverlay.
 */

#ifndef ENGINE_PROFILER_PROFILER_H_
#define ENGINE_PROFILER_PROFILER_H_

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "engine/time.h"
#include "engine/time/monotonic_time.h"

namespace engine::profiler {

/**
 * @brief Variant type for metric values
 * Supports common metric types: floating point (performance), integer (counts),
 * and string (state)
 */
using MetricValue = std::variant<float, int, std::string>;

/**
 * @struct MetricHistory
 * @brief Tracks historical values for a specific metric over time
 *
 * @details
 * Maintains a rolling window of recent values to allow plotting graphs or
 * calculating statistics. Automatically tracks min/max values for scaling.
 */
struct MetricHistory {
  /// @brief Rolling buffer of recent values
  std::deque<float> values;

  /// @brief Maximum number of samples to keep before discarding oldest
  size_t max_samples{120};

  /// @brief Minimum value currently in the buffer
  float min_value{0.0f};

  /// @brief Maximum value currently in the buffer
  float max_value{0.0f};

  /**
   * @brief Add a new sample to the history
   * @param value The floating point value to record
   *
   * @details
   * Adds the value to the history buffer. If the buffer exceeds max_samples,
   * the oldest value is removed. Efficiently updates min/max values.
   */
  void Add(float value);
};

/**
 * @class Profiler
 * @brief Singleton manager for all application profiling data
 *
 * @details
 * Thread-safe central registry for metrics. Provides methods to set instant
 * values or record samples for history. Data can be retrieved for
 * visualization.
 *
 * @example
 * @code
 * // Update a counter
 * Profiler::Get().SetMetric("EntityCount", 42);
 *
 * // Record a duration for graphical plotting
 * Profiler::Get().RecordSample("FrameTime", dt.as_seconds());
 * @endcode
 */
class Profiler {
 public:
  /**
   * @brief Access the singleton Profiler instance
   * @return Reference to the global Profiler
   */
  static Profiler& Get();

  /**
   * @brief Set an instantaneous metric value
   * @param name Unique identifier for the metric
   * @param value The current value (float, int, or string)
   *
   * @details
   * Overwrites any existing value for the given name. Used for state that
   * doesn't need history, like "Map Name" or "Player Health".
   */
  void SetMetric(const std::string& name, MetricValue value);

  /**
   * @brief Record a sample for a historical metric
   * @param name Unique identifier for the metric
   * @param value The floating point sample to add
   *
   * @details
   * Adds the sample to the MetricHistory associated with name. Also updates
   * the current instantaneous value. Useful for things like FPS, latency, etc.
   */
  void RecordSample(const std::string& name, float value);

  /**
   * @brief Retrieve a snapshot of all current metric values
   * @return A copy of the metrics map
   *
   * @note Returns by value to ensure thread safety during iteration by avoiding
   * holding internal locks while the caller processes data.
   */
  std::unordered_map<std::string, MetricValue> GetMetrics();

  /**
   * @brief Retrieve a snapshot of all metric histories
   * @return A copy of the histories map
   *
   * @note Returns by value to ensure thread safety.
   */
  std::unordered_map<std::string, MetricHistory> GetHistories();

 private:
  Profiler() = default;
  ~Profiler() = default;

  std::mutex mutex_;
  std::unordered_map<std::string, MetricValue> metrics_;
  std::unordered_map<std::string, MetricHistory> histories_;
};

/**
 * @class ScopedTimer
 * @brief RAII helper for measuring block execution time
 *
 * @details
 * Starts a timer on construction and records the elapsed time to the Profiler
 * on destruction. Useful for measuring function or scope duration.
 *
 * @example
 * @code
 * void Update() {
 *   ScopedTimer timer("UpdateLoop");
 *   // ... work ...
 * } // "UpdateLoop" metric recorded here automatically
 * @endcode
 */
class ScopedTimer {
 public:
  /**
   * @brief Start the timer
   * @param name The metric name to record to when the scope ends
   */
  ScopedTimer(std::string_view name);

  /**
   * @brief Stop the timer and record duration
   */
  ~ScopedTimer();

 private:
  std::string_view name_;
  std::chrono::steady_clock::time_point start_time_;
};

}  // namespace engine::profiler

#endif  // ENGINE_PROFILER_PROFILER_H_
