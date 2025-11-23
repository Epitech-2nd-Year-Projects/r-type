#ifndef ENGINE_TIME_FRAME_TIMER_H_
#define ENGINE_TIME_FRAME_TIMER_H_

#include "clock.h"
#include "time_delta.h"

namespace engine::time {

/**
 * @class FrameTimer
 * @brief Simple frame timer for tracking delta time
 *
 * Measures frame delta time and tracks target FPS for reference.
 *
 * @section usage Usage
 * @code
 * FrameTimer frame_timer(60.0f);
 *
 * while (game_running) {
 *   TimeDelta dt = frame_timer.tick();
 *   update(dt);
 *   render();
 * }
 * @endcode
 */
class FrameTimer {
 public:
  /**
   * @brief Create frame timer with target FPS
   * @param target_fps Target frames per second (e.g., 60)
   *
   * @example
   * @code
   * FrameTimer timer(60.0f);
   * @endcode
   */
  explicit FrameTimer(float target_fps = 60.0f);

  /**
   * @brief Update timer and return frame delta time
   * @return TimeDelta since last tick()
   *
   * Must be called once per frame.
   *
   * @example
   * @code
   * TimeDelta dt = frame_timer.tick();
   * update(dt);
   * @endcode
   */
  TimeDelta tick();

  /**
   * @brief Get current frame delta time
   * @return Last delta returned by tick()
   *
   * @example
   * @code
   * TimeDelta dt = frame_timer.delta_time();
   * @endcode
   */
  TimeDelta delta_time() const;

  /**
   * @brief Reset timer and statistics
   */
  void reset();

  /**
   * @brief Get target frame time
   * @return TimeDelta representing 1/target_fps
   *
   * @example
   * @code
   * TimeDelta target = frame_timer.target_frame_time();
   * @endcode
   */
  TimeDelta target_frame_time() const;

  /**
   * @brief Set target FPS
   * @param fps Target frames per second
   *
   * @example
   * @code
   * frame_timer.set_target_fps(120);
   * @endcode
   */
  void set_target_fps(float fps);

 private:
  Clock clock_;
  TimeDelta delta_time_;
  TimeDelta target_frame_time_;
};

}  // namespace engine::time

#endif  // ENGINE_TIME_FRAME_TIMER_H_