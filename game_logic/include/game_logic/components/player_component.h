#ifndef GAME_LOGIC_COMPONENTS_PLAYER_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_PLAYER_COMPONENT_H_

#include <cstdint>

#include "engine/math/vector2.h"

namespace game_logic::components {

/**
 * @brief Player identification and state
 *
 * @details
 * Marks entity as player-controlled. Tracks player identity,
 * game instance membership, score, lives, and visual color
 * for multiplayer distinction.
 */
struct PlayerComponent {
  /// @brief Unique player identifier (across all rooms)
  std::uint32_t player_id{0};

  /// @brief Game instance/room identifier
  std::uint32_t room_id{0};

  /// @brief Player slot index within room (0-3 for 4-player game)
  std::uint8_t player_slot{0};

  /// @brief Player color (RGB 0-255)
  struct Color {
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
  } color;

  /// @brief Current score
  std::uint32_t score{0};

  /// @brief Remaining lives
  std::uint32_t lives{3};

  PlayerComponent() = default;
  PlayerComponent(std::uint32_t id, std::uint32_t room, std::uint8_t slot)
      : player_id(id), room_id(room), player_slot(slot) {}
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_PLAYER_COMPONENT_H_