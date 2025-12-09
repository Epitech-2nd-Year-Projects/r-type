#ifndef SERVER_GAME_INSTANCE_H_
#define SERVER_GAME_INSTANCE_H_

#include <cstdint>
#include <random>
#include <unordered_map>

#include "engine/time/time_delta.h"
#include "engine/util/logging.h"
#include "protocol/input_state.h"
#include "protocol/header.h"

namespace server {

/**
 * @brief Minimal authoritative game instance owned by the dedicated server.
 *
 * This class is intentionally thin for now. It provides the integration
 * points between the network/server layer and the future gamelogic module.
 *
 * Later, it can wrap a gamelogic::GameWorld or similar and delegate all
 * gameplay-related work.
 */
class GameInstance {
 public:
  /**
   * @brief Constructs a game instance with a deterministic random seed.
   * @param seed Random seed for deterministic simulation and spawning.
   */
  explicit GameInstance(std::uint32_t seed);

  GameInstance(const GameInstance&) = delete;
  GameInstance& operator=(const GameInstance&) = delete;

  /**
   * @brief Called when a player successfully joins the game.
   * @param player_id The unique ID of the player joining.
   * 
   * Initializes player state tracking for input processing and simulation.
   * Future implementation will spawn the player entity in the game world.
   */
  void OnPlayerJoined(std::uint32_t player_id);

  /**
   * @brief Called when a player leaves or times out.
   * @param player_id The unique ID of the departing player.
   * 
   * Cleans up player state and marks them as disconnected.
   * Future implementation will despawn the player entity from the game world.
   */
  void OnPlayerLeft(std::uint32_t player_id);

  /**
   * @brief Called whenever an InputState packet is received from a player.
   * @param player_id The ID of the player sending input.
   * @param payload The input state payload containing a rolling window of commands.
   * @param header The protocol header containing timing and sequence information.
   * 
   * The payload contains a rolling window of recent InputCommand entries
   * for redundancy. Selects the newest command (highest input_sequence)
   * that has not been processed yet to handle out-of-order delivery.
   * 
   * Future implementation will apply the input to the player entity
   * and perform server-side prediction reconciliation.
   */
  void OnPlayerInput(std::uint32_t player_id,
                     const protocol::InputStatePayload& payload,
                     const protocol::Header& header);

  /**
   * @brief Advances the simulation by one tick.
   * @param delta Time elapsed since the last update.
   * 
   * Drives the game loop including:
   * - Physics and movement updates
   * - Enemy AI and spawning
   * - Projectile simulation
   * - Collision detection and resolution
   * 
   * Future implementation will delegate to gamelogic::GameWorld for
   * ECS-based entity updates and system execution.
   */
  void Update(const engine::time::TimeDelta& delta);

 private:
  /**
   * @brief Per-player state tracked by the game instance.
   * 
   * Maintains input history and timing information for:
   * - Input redundancy and deduplication
   * - Server-side prediction reconciliation
   * - Lag compensation calculations
   */
  struct PlayerState {
    std::uint32_t player_id{0};                   ///< Unique player identifier.
    bool connected{false};                        ///< Whether the player is currently connected.
    protocol::InputCommand last_command{};        ///< Most recently processed input command.
    std::uint32_t last_applied_sequence{0};       ///< Highest input sequence number applied.
    std::uint32_t last_input_client_time_ms{0};   ///< Client timestamp of last processed input.
    std::uint32_t last_input_server_time_ms{0};   ///< Server timestamp when last input was processed.
  };

  std::unordered_map<std::uint32_t, PlayerState> players_;  ///< Map of player IDs to their state.
  std::mt19937 rng_;                                        ///< Random number generator for deterministic spawning.
};

}  // namespace server

#endif  // SERVER_GAME_INSTANCE_H_
