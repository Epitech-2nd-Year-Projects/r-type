#ifndef SERVER_ROOM_H_
#define SERVER_ROOM_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include "engine/time/time_delta.h"
#include "game_instance.h"
#include "protocol/snapshot_history.h"
#include "protocol/world_snapshot.h"
#include "protocol/input_state.h"
#include "protocol/header.h"

namespace server {

/**
 * @brief Encapsulates all per-room simulation state.
 *
 * Owns an authoritative GameInstance, tracks membership, and maintains
 * snapshot history for world replication.
 */
class Room {
 public:
  /**
   * @brief Constructs a room with its own game instance and seed.
   * @param room_code Client-facing room code.
   * @param room_id Internal numeric identifier.
   * @param max_players Maximum allowed players for the room.
   * @param seed Seed for deterministic simulation.
   */
  Room(std::string room_code,
       std::uint32_t room_id,
       std::uint16_t max_players,
       std::uint32_t seed);

  Room(const Room&) = delete;
  Room& operator=(const Room&) = delete;
  Room(Room&&) = default;
  Room& operator=(Room&&) = default;

  /**
   * @brief Steps the room simulation.
   * @param delta Fixed timestep.
   */
  void Update(const engine::time::TimeDelta& delta);

  /**
   * @brief Adds a player to the room.
   * @param player_id Unique player identifier.
   * @param player_name Display name.
   */
  void AddPlayer(std::uint32_t player_id, std::string_view player_name);

  /**
   * @brief Removes a player from the room.
   * @param player_id Identifier to remove.
   */
  void RemovePlayer(std::uint32_t player_id);

  /**
   * @brief Routes an input payload to the room game instance.
   */
  void HandleInput(std::uint32_t player_id,
                   const protocol::InputStatePayload& payload,
                   const protocol::Header& header);

  /**
   * @brief Builds the next world snapshot for this room.
   * @param server_tick Current server tick.
   * @return Snapshot payload ready for encoding.
   */
  protocol::WorldSnapshotPayload BuildSnapshot(std::uint32_t server_tick);

  /**
   * @brief Marks room activity.
   * @param timestamp_ms Timestamp to record.
   */
  void MarkActive(std::uint32_t timestamp_ms);

  /**
   * @brief Returns the room code.
   */
  const std::string& Code() const;

  /**
   * @brief Returns the numeric room identifier.
   */
  std::uint32_t Id() const;

  /**
   * @brief Returns the maximum players allowed.
   */
  std::uint16_t MaxPlayers() const;

  /**
   * @brief Returns the deterministic seed used by the room.
   */
  std::uint32_t Seed() const;

  /**
   * @brief Returns current player count.
   */
  std::size_t PlayerCount() const;

  /**
   * @brief Returns last activity timestamp.
   */
  std::uint32_t LastActiveMs() const;

  /**
   * @brief Checks if the room has no players.
   */
  bool IsEmpty() const;

 private:
  std::string room_code_;
  std::uint32_t room_id_;
  std::uint16_t max_players_;
  std::uint32_t seed_;
  std::uint32_t next_snapshot_id_{1};
  std::uint32_t last_active_ms_{0};
  std::unordered_set<std::uint32_t> players_;
  std::unique_ptr<GameInstance> game_instance_;
  protocol::SnapshotHistory snapshot_history_;
};

}  // namespace server

#endif  // SERVER_ROOM_H_
