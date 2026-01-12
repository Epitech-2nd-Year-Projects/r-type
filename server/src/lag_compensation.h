#ifndef SERVER_LAG_COMPENSATION_H_
#define SERVER_LAG_COMPENSATION_H_

#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "engine/math/vector2.h"

namespace server {

/**
 * @brief Records the state of a player entity at a specific point in time.
 */
struct PlayerSnapshot {
  std::uint32_t server_tick;
  std::uint32_t timestamp_ms;
  std::uint32_t player_id;
  engine::math::Vector2f position;
};

/**
 * @brief Manages a history of player positions for Lag Compensation.
 * 
 * Used to determine where a player WAS when they fired a shot, 
 * to allow spawning projectiles at the correct visual origin.
 */
class LagCompensationHistory {
 public:
  explicit LagCompensationHistory(std::uint32_t history_duration_ms = 1000);

  /**
   * @brief Records a new snapshot of a player's state.
   * @param server_tick Current server tick.
   * @param timestamp_ms Current server time.
   * @param player_id The player's ID.
   * @param position The player's current position.
   */
  void RecordSnapshot(std::uint32_t server_tick, std::uint32_t timestamp_ms,
                      std::uint32_t player_id, engine::math::Vector2f position);

  /**
   * @brief Retrieves the interpolated position of a player at a past time.
   * @param player_id The player to query.
   * @param target_time_ms The timestamp to look up (usually client_time).
   * @return The interpolated position, or nullopt if no history exists.
   */
  std::optional<engine::math::Vector2f> GetPlayerPositionAt(
      std::uint32_t player_id, std::uint32_t target_time_ms) const;

 private:
  // We keep a separate history deque for each player for faster lookup.
  struct PlayerHistory {
    std::deque<PlayerSnapshot> snapshots;
  };

  std::unordered_map<std::uint32_t, PlayerHistory> history_;
  std::uint32_t max_history_duration_ms_;

  void PruneOldSnapshots(PlayerHistory& ph, std::uint32_t current_time_ms);
};

}  // namespace server

#endif  // SERVER_LAG_COMPENSATION_H_
