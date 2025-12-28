#ifndef GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_

#include <string>

#include "engine/math/vector2.h"

namespace game_logic::components {

/**
 * @brief Enemy AI behavior configuration
 *
 * @details
 * Defines enemy movement patterns, patrol areas, speeds,
 * and targeting behavior. AISystem interprets this data.
 */
struct AIComponent {
  /// @brief Current behavior pattern name (maps to Lua function)
  std::string behavior_name{"Straight"};

  /// @brief Movement speed (pixels/second)
  float speed{50.0f};

  /// @brief Patrol area minimum bounds
  engine::math::Vector2f patrol_min{0.0f, 0.0f};

  /// @brief Patrol area maximum bounds
  engine::math::Vector2f patrol_max{800.0f, 600.0f};

  /// @brief Player detection range (0 = infinite)
  float detection_range{0.0f};

  /// @brief Internal state timer (for wave patterns, etc.)
  float state_timer{0.0f};

  /// @brief Wave amplitude (for wave pattern)
  float wave_amplitude{50.0f};

  /// @brief Wave frequency (for wave pattern)
  float wave_frequency{2.0f};

  AIComponent() = default;
  explicit AIComponent(std::string b) : behavior_name(std::move(b)) {}
  AIComponent(std::string b, float spd)
      : behavior_name(std::move(b)), speed(spd) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_