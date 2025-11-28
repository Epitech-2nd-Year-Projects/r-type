#ifndef GAME_LOGIC_GAME_INSTANCE_H_
#define GAME_LOGIC_GAME_INSTANCE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"
#include "game_logic/game_state.h"

namespace game_logic {

/**
 * @class GameInstance
 * @brief Manages a single R-Type game match
 *
 * @details
 * GameInstance orchestrates one game session (room) including:
 * - Player lifecycle (join/leave)
 * - ECS Registry ownership
 * - Component and system registration
 * - Game state tracking (level, wave, scores)
 * - Game flow control (start/update/shutdown)
 *
 * Each server can host multiple GameInstance objects representing
 * different concurrent matches.
 */
class GameInstance {
 public:
  /**
   * @brief Create game instance
   * @param room_id Unique room identifier
   * @param max_players Maximum number of players (default: 4)
   */
  explicit GameInstance(std::uint32_t room_id, std::uint32_t max_players = 4);

  /**
   * @brief Destructor (calls Shutdown if not already called)
   */
  ~GameInstance();

  /// @brief Non-copyable
  GameInstance(const GameInstance&) = delete;
  GameInstance& operator=(const GameInstance&) = delete;

  /**
   * @brief Initialize and start the game
   *
   * @details
   * - Initializes game world
   * - Sets level and wave to 1
   * - Marks game as running
   *
   * Safe to call multiple times (subsequent calls ignored).
   */
  void Start();

  /**
   * @brief Update game simulation
   * @param dt Time since last frame
   *
   * @details
   * - Runs all registered ECS systems
   * - Updates player scores from components
   * - Checks win/loss conditions
   * - Updates GameState
   */
  void Update(engine::time::TimeDelta dt);

  /**
   * @brief Stop game and release resources
   *
   * @details
   * - Marks game as not running
   * - Clears all systems
   * - Removes all players
   * - Resets state
   *
   * Safe to call multiple times.
   */
  void Shutdown();

  /**
   * @brief Add player to game instance
   * @param player_id Unique player identifier
   * @param player_name Display name
   *
   * @details
   * Does nothing if:
   * - Max players reached
   * - Player already exists
   */
  void AddPlayer(std::uint32_t player_id, std::string_view player_name);

  /**
   * @brief Remove player from game instance
   * @param player_id Player to remove
   *
   * @details
   * Safe to call on non-existent player (no-op).
   */
  void RemovePlayer(std::uint32_t player_id);

  /**
   * @brief Get mutable access to ECS Registry
   * @return Reference to internal registry
   *
   * @details
   * Use this to:
   * - Spawn entities
   * - Add/remove components
   * - Query component data
   */
  engine::ecs::Registry& World();

  /**
   * @brief Get const access to ECS Registry
   */
  const engine::ecs::Registry& World() const;

  /**
   * @brief Get current game state (read-only)
   * @return Const reference to state snapshot
   */
  const GameState& State() const;

  /**
   * @brief Get mutable game state
   * @return Reference to state (use with caution)
   */
  GameState& State();

  /**
   * @brief Check if game is actively running
   * @return true if started and not finished
   */
  bool IsRunning() const;

  /**
   * @brief Check if game has finished
   * @return true if game over (win or loss)
   */
  bool IsFinished() const;

  /**
   * @brief Get room identifier
   */
  std::uint32_t RoomId() const;

  /**
   * @brief Get maximum player capacity
   */
  std::uint32_t MaxPlayers() const;

  /**
   * @brief Get current number of active players
   */
  std::uint32_t ActivePlayerCount() const;

 private:
  /**
   * @brief Register all component types with Registry
   *
   * @details
   * Registers both engine and game_logic components.
   * Called once during construction.
   */
  void RegisterComponents();

  /**
   * @brief Register all systems with Registry
   *
   * @details
   * Sets up MovementSystem, LifetimeSystem, etc.
   * Called once during construction.
   */
  void RegisterSystems();

  /**
   * @brief Initialize game world (spawn initial entities)
   *
   * @details
   * Called by Start(). Will be expanded in future tickets
   * with entity builders and wave system.
   */
  void InitializeGame();

  /**
   * @brief Sync GameState with ECS component data
   *
   * @details
   * Updates player_scores from PlayerComponent data.
   * Checks game over conditions.
   * Called each frame by Update().
   */
  void UpdateGameState();

  /// @brief Unique room identifier
  std::uint32_t room_id_;

  /// @brief Maximum allowed players
  std::uint32_t max_players_;

  /// @brief Owned ECS registry
  std::unique_ptr<engine::ecs::Registry> registry_;

  /// @brief Current game state snapshot
  GameState game_state_;

  /// @brief Player ID -> player name mapping
  std::unordered_map<std::uint32_t, std::string> player_names_;

  /// @brief Whether Start() has been called
  bool is_started_;
};

}  // namespace game_logic

#endif  // GAME_LOGIC_GAME_INSTANCE_H_