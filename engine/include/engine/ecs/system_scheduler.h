/**
 * @file system_scheduler.h
 * @brief Internal system scheduling and execution (PRIVATE)
 * @version 1.0.0
 *
 * @details
 * This is an internal implementation detail. Users should not include
 * this header directly - use Registry instead.
 *
 * SystemScheduler manages:
 * - System registration and storage
 * - Execution ordering by priority
 * - Fixed vs. variable timestep accumulation
 * - Per-frame system execution
 */

#ifndef ENGINE_ECS_INTERNAL_SYSTEM_SCHEDULER_H_
#define ENGINE_ECS_INTERNAL_SYSTEM_SCHEDULER_H_

#include <functional>
#include <memory>
#include <vector>

#include "engine/time/time_delta.h"
#include "system.h"

namespace engine::ecs {

// Forward declaration
class Registry;

/**
 * @class SystemEntry
 * @brief Internal system wrapper (PRIVATE)
 *
 * Stores a system function/object with its metadata (type, priority).
 */
struct SystemEntry {
  /// @brief System execution function
  std::function<void(Registry&, time::TimeDelta)> function;

  /// @brief System type (fixed or variable)
  SystemType type;

  /// @brief Execution priority
  SystemPriority priority;

  /**
   * @brief Construct system entry
   * @param fn System function
   * @param t System type
   * @param p Priority
   */
  SystemEntry(std::function<void(Registry&, time::TimeDelta)> fn, SystemType t,
              SystemPriority p)
      : function(std::move(fn)), type(t), priority(p) {}
};

/**
 * @class SystemScheduler
 * @brief Manages system execution and timing (PRIVATE)
 *
 * @details
 * Responsibilities:
 * - Store registered systems
 * - Sort by priority
 * - Accumulate fixed timestep
 * - Execute systems in correct order
 *
 * @section timing Fixed Timestep Accumulation
 * Fixed systems accumulate frame time and execute multiple times if needed:
 * @code
 * accumulator += frame_dt;
 * while (accumulator >= fixed_step) {
 *   run_fixed_systems(fixed_step);
 *   accumulator -= fixed_step;
 * }
 * @endcode
 */
class SystemScheduler {
 public:
  /**
   * @brief Create scheduler with default fixed timestep
   * @param fixed_timestep Timestep for fixed systems (default: 1/60s)
   */
  explicit SystemScheduler(time::TimeDelta fixed_timestep =
                               time::TimeDelta::from_seconds(1.0f / 60.0f));

  /**
   * @brief Register a system
   * @param system_fn System function to execute
   * @param type System type (fixed or variable)
   * @param priority Execution priority (higher = earlier)
   */
  void RegisterSystem(std::function<void(Registry&, time::TimeDelta)> system_fn,
                      SystemType type, SystemPriority priority);

  /**
   * @brief Execute all systems for one frame
   * @param registry Registry reference
   * @param dt Time since last frame
   *
   * @details
   * - Runs all variable systems once with `dt`
   * - Accumulates time for fixed systems
   * - Runs fixed systems 0-N times depending on accumulator
   */
  void Update(Registry& registry, time::TimeDelta dt);

  /**
   * @brief Set fixed timestep interval
   * @param timestep New fixed timestep
   */
  void SetFixedTimestep(time::TimeDelta timestep);

  /**
   * @brief Get current fixed timestep
   * @return Fixed timestep duration
   */
  time::TimeDelta FixedTimestep() const;

  /**
   * @brief Clear all registered systems
   */
  void Clear();

 private:
  /**
   * @brief Sort systems by priority (descending)
   */
  void SortSystems();

  /// @brief Registered systems
  std::vector<SystemEntry> systems_;

  /// @brief Fixed timestep interval
  time::TimeDelta fixed_timestep_;

  /// @brief Accumulated time for fixed updates
  time::TimeDelta accumulator_;

  /// @brief Whether systems need re-sorting
  bool needs_sort_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_INTERNAL_SYSTEM_SCHEDULER_H_