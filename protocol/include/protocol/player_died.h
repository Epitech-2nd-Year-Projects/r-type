#ifndef PROTOCOL_PLAYER_DIED_H_
#define PROTOCOL_PLAYER_DIED_H_

#include <cstdint>

#include "engine/net/packet_buffer.h"

namespace protocol {

/**
 * @brief Cause of death for a player.
 * 
 * This is mostly for UI / feedback, not strict gameplay logic.
 */
enum class DeathCause : std::uint8_t {
  kUnknown = 0,     ///< Unknown cause.
  kEnemy = 1,       ///< Killed by enemy.
  kProjectile = 2,  ///< Killed by projectile.
  kObstacle = 3,    ///< Killed by obstacle.
  kSuicide = 4,     ///< Self-inflicted death.
  kVoid = 5,        ///< Fell off-screen / out of bounds.
};

/**
 * @brief Server → Client: notify that a player has died.
 * 
 * Wire format:
 *   - uint32  player_id
 *   - uint32  killer_entity_id
 *   - uint8   cause
 *   - uint8   remaining_lives
 * 
 * @note killer_entity_id can be 0 if there is no clear killer (e.g. environment).
 */
struct PlayerDiedPayload {
  std::uint32_t player_id = 0;         ///< ID of the player who died.
  std::uint32_t killer_entity_id = 0;  ///< Entity ID of the killer (0 if no clear killer).
  DeathCause cause = DeathCause::kUnknown;  ///< Cause of death.
  std::uint8_t remaining_lives = 0;    ///< Number of lives remaining for the player.
};

/**
 * @brief Encodes a PlayerDiedPayload into the buffer.
 * @param payload The player died payload to serialize.
 * @param buffer The packet buffer to write to.
 */
bool EncodePlayerDied(const PlayerDiedPayload& payload, engine::net::PacketBuffer& buffer);

/**
 * @brief Decodes a PlayerDiedPayload from the buffer.
 * @param buffer The packet buffer to read from.
 * @param out_payload Output parameter for the deserialized player died payload.
 * @return true on success, false if the buffer is too small or invalid.
 */
bool DecodePlayerDied(engine::net::PacketBuffer& buffer, PlayerDiedPayload& out_payload);

}  // namespace protocol

#endif  // PROTOCOL_PLAYER_DIED_H_
