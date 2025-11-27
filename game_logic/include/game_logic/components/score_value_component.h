#ifndef GAME_LOGIC_COMPONENTS_SCORE_VALUE_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_SCORE_VALUE_COMPONENT_H_

#include <cstdint>

namespace game_logic::components {

/**
 * @brief Points awarded when entity destroyed
 *
 * @details
 * Typically attached to enemies. Scoring system grants points
 * to player who destroyed the entity.
 */
struct ScoreValueComponent {
  /// @brief Points awarded on death
  std::uint32_t points{100};

  /// @brief Whether points already claimed (prevent double-scoring)
  bool claimed{false};

  ScoreValueComponent() = default;
  explicit ScoreValueComponent(std::uint32_t pts) : points(pts) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_SCORE_VALUE_COMPONENT_H_