#ifndef SERVER_GAME_INSTANCE_H_
#define SERVER_GAME_INSTANCE_H_

#include <cstdint>
#include <random>
#include <unordered_map>
#include <optional>
#include <functional>

#include "engine/time/time_delta.h"
#include "engine/util/logging.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/components/tag_component.h"
#include "rift/components/fighter_component.h"
#include "rift/game_instance.h"
#include "protocol/command.h"
#include "protocol/input_state.h"
#include "protocol/header.h"
#include "protocol/world_snapshot.h"
#include "protocol/snapshot_history.h"

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
  * @param room_id Logical room identifier associated with this instance.
  * @param seed Random seed for deterministic simulation and spawning.
  * @param max_players Maximum number of allowed players for this instance.
  * @param logger Logger used for diagnostics.
  */
  explicit GameInstance(std::uint32_t room_id,
                        std::uint32_t seed,
                        std::uint32_t max_players,
                        engine::util::Logger& logger);

  GameInstance(const GameInstance&) = delete;
  GameInstance& operator=(const GameInstance&) = delete;

  /**
   * @brief Called when a player successfully joins the game.
   * @param player_id The unique ID of the player joining.
   * 
   * Initializes player state tracking for input processing and simulation.
   * Future implementation will spawn the player entity in the game world.
   */
  void OnPlayerJoined(std::uint32_t player_id, std::string_view player_name);

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
   * @brief Handles reliable client commands (e.g., ready/unready).
   * @return ReadyEvent if state changed / game started, std::nullopt otherwise.
   */
  struct ReadyEvent {
    std::uint32_t player_id{0};
    bool is_ready{false};
    bool game_started{false};
  };
  std::optional<ReadyEvent> OnClientCommand(std::uint32_t player_id,
                                            const protocol::CommandPayload& command);

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

  /**
   * @brief Build a world snapshot for network replication.
   *
   * This is called once per server tick by ServerRuntime. The snapshot
   * is then broadcast to all joined peers.
   *
   * @param snapshot_id Monotonically increasing snapshot identifier.
   * @param server_tick Current server tick counter.
   * @return A WorldSnapshotPayload ready to be serialized by protocol::Encode.
   *
   * @note In this “minimal” implementation, we only fill the header fields
   *       (snapshot_id, base_snapshot_id, server_tick) and leave the deltas
   *       vector empty. A later issue will plug the real ECS/entity state.
   */
  [[nodiscard]] protocol::WorldSnapshotPayload BuildWorldSnapshot(std::uint32_t snapshot_id, std::uint32_t server_tick);

  /**
   * @brief Returns a mutable reference to the ECS registry.
   * @return Reference to the entity-component-system registry.
   * 
   * Provides direct access to the underlying ECS world for entity and
   * component management. Use for spawning, destroying, or querying entities.
   */
  engine::ecs::Registry& World();

  /**
   * @brief Returns a const reference to the ECS registry.
   * @return Const reference to the entity-component-system registry.
   * 
   * Provides read-only access to the ECS world for queries and inspection
   * without allowing modifications.
   */
  const engine::ecs::Registry& World() const;

  /**
   * @brief Returns a mutable reference to the game logic instance.
   * @return Reference to the game logic subsystem.
   * 
   * Provides access to the gameplay rules, systems, and logic layer.
   * Use for triggering game events, managing game state, or invoking
   * gameplay-specific operations.
   */
  rift::GameInstance& Logic();

  /**
   * @brief Returns a const reference to the game logic instance.
   * @return Const reference to the game logic subsystem.
   *
   * Provides read-only access to the game logic layer for queries
   * and inspection without allowing modifications.
   */
  const rift::GameInstance& Logic() const;

  private:
  /**
   * @brief Resolves the entity type identifier for network serialization.
   * @param tag The entity's tag component (may be nullptr).
   * @param player The entity's player component (may be nullptr).
   * @return The entity type ID for the WorldSnapshotPayload.
   * 
   * Maps ECS component combinations to protocol entity types.
   * Used during snapshot building to classify entities for network replication.
   * Handles player entities, enemies, projectiles, and other game objects.
   */
  std::uint16_t ResolveEntityType(
      std::optional<std::reference_wrapper<const engine::ecs::TagComponent>>
          tag,
      std::optional<
          std::reference_wrapper<const rift::components::FighterComponent>>
          fighter) const;
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
    std::uint8_t last_buttons{0};                     ///< Button bitfield of last processed input.
    bool is_ready{false};                          ///< Lobby ready flag.
  };

  std::unordered_map<std::uint32_t, PlayerState> players_;  ///< Map of player IDs to their state.
  std::mt19937 rng_;                                        ///< Random number generator for deterministic spawning.
  std::unique_ptr<rift::GameInstance> logic_;               ///< Game logic subsystem managing gameplay rules and systems.
  engine::util::Logger& logger_;                            ///< Logger shared with the server runtime.

  enum class Phase {
    kLobby,
    kPlaying
  };
  Phase phase_{Phase::kLobby};                              ///< Lobby vs active play.

  bool CheckStartCondition() const;
};

}  // namespace server

#endif  // SERVER_GAME_INSTANCE_H_
