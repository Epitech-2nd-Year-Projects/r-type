#ifndef GAME_LOGIC_DIFFICULTY_H_
#define GAME_LOGIC_DIFFICULTY_H_

#include <array>
#include <cstdint>
#include <string_view>

namespace game_logic {

/**
 * @brief Game difficulty levels.
 *
 * @details
 * Difficulty affects enemy stats (speed, health, damage, fire rate)
 * and player stats (health, lives). This enum is independent from
 * the protocol layer to maintain layer separation.
 */
enum class Difficulty : std::uint8_t {
  kEasy = 0,
  kNormal = 1,
  kHard = 2,
  kHardcore = 3
};

/// @brief Total number of difficulty levels.
inline constexpr std::size_t kDifficultyCount = 4;

/// @brief String names for each difficulty level.
inline constexpr std::array<std::string_view, kDifficultyCount>
    kDifficultyNames = {"Easy", "Normal", "Hard", "Hardcore"};

/**
 * @brief Convert difficulty enum to string name.
 * @param difficulty The difficulty level.
 * @return String view of the difficulty name.
 */
inline std::string_view DifficultyToString(Difficulty difficulty) {
  const auto index = static_cast<std::size_t>(difficulty);
  if (index < kDifficultyNames.size()) {
    return kDifficultyNames[index];
  }
  return kDifficultyNames[static_cast<std::size_t>(Difficulty::kNormal)];
}

}  // namespace game_logic

#endif  // GAME_LOGIC_DIFFICULTY_H_
