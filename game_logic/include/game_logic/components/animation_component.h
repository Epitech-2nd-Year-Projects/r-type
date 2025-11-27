#ifndef GAME_LOGIC_COMPONENTS_ANIMATION_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_ANIMATION_COMPONENT_H_

#include <cstddef>
#include <vector>

#include "engine/math/rect.h"
#include "engine/time/time_delta.h"

namespace game_logic::components {

/**
 * @brief Frame-based sprite animation
 *
 * @details
 * Stores animation frames as source rectangles. AnimationSystem
 * updates SpriteComponent's source_rect based on frame timing.
 */
struct AnimationComponent {
  /// @brief Animation frames (source rects in sprite sheet)
  std::vector<engine::math::RectF> frames;

  /// @brief Duration per frame
  engine::time::TimeDelta frame_duration{
      engine::time::TimeDelta::from_seconds(0.1f)};

  /// @brief Current frame index
  std::size_t current_frame{0};

  /// @brief Elapsed time in current frame
  engine::time::TimeDelta elapsed{engine::time::TimeDelta::zero()};

  /// @brief Loop animation when reaching end
  bool looping{true};

  /// @brief Animation is playing
  bool playing{true};

  AnimationComponent() = default;
  explicit AnimationComponent(std::vector<engine::math::RectF> anim_frames)
      : frames(std::move(anim_frames)) {}

  /**
   * @brief Reset animation to first frame
   */
  void reset() {
    current_frame = 0;
    elapsed = engine::time::TimeDelta::zero();
  }

  /**
   * @brief Check if animation finished (non-looping only)
   */
  bool is_finished() const {
    return !looping && current_frame >= frames.size() - 1;
  }
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_ANIMATION_COMPONENT_H_