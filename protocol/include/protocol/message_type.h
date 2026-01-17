#ifndef PROTOCOL_MESSAGE_TYPE_H_
#define PROTOCOL_MESSAGE_TYPE_H_

#include <cstdint>

namespace protocol::message_type {

/**
 * @brief Network message types used in the R-Type UDP protocol.
 * 
 * These values are part of the on-the-wire contract and must stay stable
 * once the protocol is released.
 */
enum class MessageType : std::uint8_t {
  kInvalid = 0,  ///< Reserved / not used.

  // Connection / session.
  kHello = 1,          ///< Optional connectionless hello / ping server.
  kJoinRequest = 2,    ///< Client asks to join a game.
  kJoinAccept = 3,     ///< Server accepts and assigns player_id.
  kJoinReject = 4,     ///< Server rejects with a reason code.

  // Gameplay.
  kInputState = 5,     ///< Client → Server: player input commands.
  kWorldSnapshot = 6,  ///< Server → Client: world state snapshot (full or delta).
  kSpawnEntity = 7,    ///< Server → Client: explicit spawn (optional).
  kDestroyEntity = 8,  ///< Server → Client: explicit destroy (optional).
  kPlayerDied = 9,     ///< Server → Client: notification that a player died.
  kGameplayPing = 18,  ///< Client <-> Server: gameplay ping (contextual).

  // Generic commands / events.
  kClientCommand = 10,  ///< Client → Server: generic reliable commands.
  kServerCommand = 11,  ///< Server → Client: generic reliable commands.

  // Utility.
  kPing = 12,  ///< Client → Server: ping with timestamp.
  kPong = 13,  ///< Server → Client: pong echoing data.

  // Lobby / room directory.
  kRoomListRequest = 14,   ///< Client → Server: request room catalog.
  kRoomListResponse = 15,  ///< Server → Client: response with room catalog.
  kCreateRoomRequest = 16, ///< Client → Server: create a room with options.
  kCreateRoomResponse = 17 ///< Server → Client: room creation result.
};
}

#endif // !PROTOCOL_MESSAGE_TYPE_H_
