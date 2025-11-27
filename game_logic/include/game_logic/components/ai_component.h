#ifndef GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_

#include <cstdint>

#include "engine/math/vector2.h"

namespace game_logic::components {

/**
 * @enum EnemyBehavior
 * @brief Enemy AI behavior patterns
 */
enum class EnemyBehavior : std::uint8_t {
  kIdle = 0,         ///< Stationary
  kStraight = 1,     ///< Move in straight line
  kPatrol = 2,       ///< Patrol between two points
  kWavePattern = 3,  ///< Sine wave movement
  kChasePlayer = 4,  ///< Follow nearest player
  kCircle = 5,       ///< Circular pattern
  kZigZag = 6        ///< Zigzag pattern
};

/**
 * @brief Enemy AI behavior configuration
 *
 * @details
 * Defines enemy movement patterns, patrol areas, speeds,
 * and targeting behavior. AISystem interprets this data.
 */
struct AIComponent {
  /// @brief Current behavior pattern
  EnemyBehavior behavior{EnemyBehavior::kStraight};

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
  explicit AIComponent(EnemyBehavior b) : behavior(b) {}
  AIComponent(EnemyBehavior b, float spd) : behavior(b), speed(spd) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_AI_COMPONENT_H_