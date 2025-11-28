#ifndef GAME_LOGIC_GAME_STATE_H_
#define GAME_LOGIC_GAME_STATE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace game_logic {

/**
 * @brief Player score and status information
 *
 * @details
 * Snapshot of a player's current state within a game instance.
 * Updated by GameInstance each frame based on ECS component data.
 */
struct PlayerScore {
  /// @brief Unique player identifier
  std::uint32_t player_id{0};

  /// @brief Player display name
  std::string name;

  /// @brief Current score
  std::uint32_t score{0};

  /// @brief Remaining lives
  std::uint32_t lives{3};

  /// @brief Whether player is currently alive
  bool is_alive{true};

  PlayerScore() = default;
  explicit PlayerScore(std::uint32_t id, std::string n)
      : player_id(id), name(std::move(n)) {}
};

/**
 * @brief Complete game instance state
 *
 * @details
 * Represents the current status of a single game match.
 * Read-only snapshot updated by GameInstance::Update().
 */
struct GameState {
  /// @brief Unique room/instance identifier
  std::uint32_t room_id{0};

  /// @brief Current level number (1-based)
  std::uint32_t current_level{0};

  /// @brief Current wave within level (1-based)
  std::uint32_t current_wave{0};

  /// @brief Scores for all players in this instance
  std::vector<PlayerScore> player_scores;

  /// @brief List of active player IDs
  std::vector<std::uint32_t> active_player_ids;

  /// @brief Game is actively running
  bool is_running{false};

  /// @brief Game has ended (win or loss)
  bool is_game_over{false};

  GameState() = default;
};

}  // namespace game_logic

#endif  // GAME_LOGIC_GAME_STATE_H_