#ifndef SERVER_ROOM_H_
#define SERVER_ROOM_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include "engine/time/time_delta.h"
#include "engine/util/logging.h"
#include "game_instance.h"
#include "protocol/command.h"
#include "protocol/header.h"
#include "protocol/input_state.h"
#include "protocol/lobby.h"
#include "protocol/player_died.h"
#include "protocol/snapshot_history.h"
#include "protocol/world_snapshot.h"

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
   * @param is_private Whether the room requires a private access code.
   * @param seed Seed for deterministic simulation.
   * @param difficulty Difficulty level for this room.
   * @param logger Logger used for diagnostics.
   */
  Room(std::string room_code, std::string room_name, std::uint32_t room_id,
       std::uint16_t max_players, bool is_private, std::string password,
       std::uint32_t seed, protocol::Difficulty difficulty,
       engine::util::Logger& logger);

  Room(const Room&) = delete;
  Room& operator=(const Room&) = delete;
  Room(Room&& other) noexcept;
  Room& operator=(Room&&) = delete;

  /**
   * @brief Steps the room simulation and advances the room tick counter.
   * @param delta Fixed timestep.
   */
  void Update(const engine::time::TimeDelta& delta);

  /**
   * @brief Adds a player to the room.
   * @param player_id Unique player identifier.
   * @param player_name Display name.
   * @return true if the player was added, false on duplicate or capacity hit.
   */
  bool AddPlayer(std::uint32_t player_id, std::string_view player_name);

  /**
   * @brief Removes a player from the room.
   * @param player_id Identifier to remove.
   */
  bool RemovePlayer(std::uint32_t player_id);

  /**
   * @brief Routes an input payload to the room game instance.
   */
  void HandleInput(std::uint32_t player_id,
                   const protocol::InputStatePayload& payload,
                   const protocol::Header& header);

  /**
   * @brief Routes a reliable client command (e.g., ready toggle).
   * @return Optional ready event for broadcasting.
   */
  std::optional<GameInstance::ReadyEvent> HandleClientCommand(
      std::uint32_t player_id, const protocol::CommandPayload& command);

  /**
   * @brief Builds the next world snapshot for this room.
   * @return Snapshot payload ready for encoding.
   */
  protocol::WorldSnapshotPayload BuildSnapshot(std::uint32_t server_tick);

  /**
   * @brief Marks room activity.
   * @param timestamp_ms Timestamp to record.
   */
  void MarkActive(std::uint32_t timestamp_ms);

  /**
   * @brief Poll any player death events that occurred since last poll
   */
  std::vector<protocol::PlayerDiedPayload> PollPlayerDeaths();

  /**
   * @brief Retrieves a snapshot from the room's history.
   */
  std::optional<std::reference_wrapper<const protocol::WorldSnapshotPayload>>
  GetSnapshot(std::uint32_t snapshot_id) const;

  /**
   * @brief Returns the room code.
   */
  const std::string& Code() const;
  const std::string& Name() const;

  /**
   * @brief Returns the numeric room identifier.
   */
  std::uint32_t Id() const;

  /**
   * @brief Returns the maximum players allowed.
   */
  std::uint16_t MaxPlayers() const;

  /**
   * @brief Whether the room is private.
   */
  bool IsPrivate() const;
  const std::string& Password() const;

  /**
   * @brief Whether the room has already started gameplay.
   */
  bool HasStarted() const;

  /**
   * @brief Returns the deterministic seed used by the room.
   */
  std::uint32_t Seed() const;

  /**
   * @brief Returns the room's difficulty level.
   */
  protocol::Difficulty Difficulty() const;

  /**
   * @brief Returns current player count.
   */
  std::size_t PlayerCount() const;

  /**
   * @brief Returns the player IDs currently in the room.
   */
  const std::unordered_set<std::uint32_t>& Players() const;

  /**
   * @brief Returns last activity timestamp.
   */
  std::uint32_t LastActiveMs() const;

  /**
   * @brief Checks if the room has no players.
   */
  bool IsEmpty() const;

  /**
   * @brief Returns a mutable reference to the game instance's registry.
   */
  engine::ecs::Registry& World();

  /**
   * @brief Returns a mutable reference to the game logic instance.
   */
  game_logic::GameInstance& Logic();

 private:
  std::string room_code_;
  std::string room_name_;
  std::uint32_t room_id_;
  std::uint16_t max_players_;
  bool is_private_;
  std::string password_;
  std::uint32_t seed_;
  protocol::Difficulty difficulty_;
  std::uint32_t next_snapshot_id_{1};
  std::uint32_t room_tick_{0};
  std::uint32_t last_active_ms_{0};
  bool started_{false};
  std::unordered_set<std::uint32_t> players_;
  std::unique_ptr<GameInstance> game_instance_;
  protocol::SnapshotHistory snapshot_history_;
  engine::util::Logger& logger_;
};

}  // namespace server

#endif  // SERVER_ROOM_H_
