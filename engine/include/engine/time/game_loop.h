#ifndef ENGINE_TIME_GAME_LOOP_H_
#define ENGINE_TIME_GAME_LOOP_H_

#include <functional>

#include "frame_timer.h"
#include "time_delta.h"

namespace engine::time {

/**
 * @class VariableTimestepLoop
 * @brief Game loop with variable timestep (render-bound)
 *
 * Pattern:
 * - Update and render every frame with actual delta time
 * - Suitable for: networked games with client prediction
 *
 * @section usage Usage
 * @code
 * VariableTimestepLoop loop(60.0f);
 *
 * loop.run([&](TimeDelta dt) {
 *   update(dt);
 *   render();
 *   return !should_quit;  // Return false to exit
 * });
 * @endcode
 */
class VariableTimestepLoop {
 public:
  /**
   * @brief Create variable timestep loop
   * @param target_fps Target display FPS (for statistics only)
   */
  explicit VariableTimestepLoop(float target_fps = 60.0f);

  /**
   * @brief Run the game loop
   * @param callback Function called each frame with delta time
   *
   * Callback signature: bool(TimeDelta dt)
   * Return false from callback to exit loop
   *
   * @example
   * @code
   * loop.run([&](TimeDelta dt) {
   *   update(dt);
   *   render();
   *   return !should_quit;
   * });
   * @endcode
   */
  void run(std::function<bool(TimeDelta)> callback);

  /**
   * @brief Single iteration of loop (non-blocking)
   * @param callback Function called this frame
   * @return true to continue, false to stop
   *
   * Use this if you need control over the loop
   */
  bool tick(std::function<bool(TimeDelta)> callback);

  /**
   * @brief Get frame timer (read-only)
   */
  const FrameTimer& frame_timer() const { return frame_timer_; }

 private:
  FrameTimer frame_timer_;
};

/**
 * @class FixedTimestepLoop
 * @brief Game loop with fixed timestep (logic-bound)
 *
 * Pattern:
 * - Accumulate frame delta
 * - Call update() multiple times with fixed timestep
 * - Call render() once per frame
 *
 * Suitable for: Physics-heavy games, networked multiplayer (server)
 *
 * @section usage Usage
 * @code
 * FixedTimestepLoop loop(
 *   TimeDelta::from_seconds(1.0f / 60.0f),  // 60 Hz physics
 *   144.0f                                   // 144 FPS display target
 * );
 *
 * loop.run([&](TimeDelta fixed_dt, TimeDelta frame_dt) {
 *   update_physics(fixed_dt);  // May be called 0-N times per frame
 *   render(frame_dt);          // May be called 0 or more times per frame,
 * depending on return !should_quit;
 * });
 * @endcode
 */
class FixedTimestepLoop {
 public:
  /**
   * @brief Create fixed timestep loop
   * @param fixed_timestep Fixed timestep for logic
   * @param target_fps Target display FPS
   */
  FixedTimestepLoop(TimeDelta fixed_timestep = TimeDelta::from_seconds(1.0f /
                                                                       60.0f),
                    float target_fps = 60.0f);

  /**
   * @brief Run the game loop
   * @param callback Function called with (fixed_dt, frame_dt)
   *
   * Callback signature: bool(TimeDelta fixed_dt, TimeDelta frame_dt)
   *
   * @example
   * @code
   * loop.run([&](TimeDelta fixed_dt, TimeDelta frame_dt) {
   *   update_physics(fixed_dt);
   *   render();
   *   return !should_quit;
   * });
   * @endcode
   */
  void run(std::function<bool(TimeDelta, TimeDelta)> callback);

  /**
   * @brief Single iteration (non-blocking)
   */
  bool tick(std::function<bool(TimeDelta, TimeDelta)> callback);

  /**
   * @brief Set fixed timestep
   */
  void set_fixed_timestep(TimeDelta timestep);

  /**
   * @brief Get fixed timestep
   */
  TimeDelta fixed_timestep() const { return fixed_timestep_; }

  /**
   * @brief Get frame timer
   */
  const FrameTimer& frame_timer() const { return frame_timer_; }

 private:
  TimeDelta fixed_timestep_;
  FrameTimer frame_timer_;
  TimeDelta accumulated_time_;
};

}  // namespace engine::time

#endif  // ENGINE_TIME_GAME_LOOP_H_