#ifndef PROTOCOL_COMMAND_H_
#define PROTOCOL_COMMAND_H_

#include <cstdint>
#include <string>

#include "engine/net/packet_buffer.h"

namespace protocol {

/**
 * @brief Optional enumeration for well-known commands.
 * 
 * The actual on-the-wire value is stored as a uint16_t command_id.
 * You can extend this later as needed.
 */
enum class CommandType : std::uint16_t {
  kUnknown = 0,      ///< Unknown command.
  kStartGame = 1,    ///< Start the game.
  kSetReady = 2,     ///< Player ready status.
  kUnready = 3,      ///< Player unready status.
  kChatMessage = 4,  ///< Chat message.
  kDisconnectNotice = 5,  ///< Server-initiated disconnect notice.
};

/**
 * @brief Generic command payload structure.
 * 
 * Contains a command identifier and an arbitrary string payload.
 */
struct CommandPayload {
  std::uint16_t command_id = 0;  ///< Command identifier.
  std::string payload;           ///< Command payload data (arbitrary string).
};

/// @brief Alias for client-to-server command payload.
using ClientCommandPayload = CommandPayload;

/// @brief Alias for server-to-client command payload.
using ServerCommandPayload = CommandPayload;

/**
 * @brief Encodes a ClientCommandPayload into a PacketBuffer.
 * @param command The client command to serialize.
 * @param buffer The packet buffer to write to.
 * @return true on success, false if the buffer runs out of space.
 */
bool EncodeCommand(const CommandPayload& command,
                   engine::net::PacketBuffer& buffer);

/**
 * @brief Decodes a ClientCommandPayload from a PacketBuffer.
 * @param buffer The packet buffer to read from.
 * @param out_command Output parameter for the deserialized client command.
 * @return true on success, false if the buffer is too small or invalid.
 */
bool DecodeCommand(engine::net::PacketBuffer& buffer,
                   CommandPayload& out_command);

// Legacy aliases (client/server share the same wire format).
inline bool EncodeClientCommand(const ClientCommandPayload& command,
                                engine::net::PacketBuffer& buffer) {
  return EncodeCommand(command, buffer);
}

inline bool DecodeClientCommand(engine::net::PacketBuffer& buffer,
                                ClientCommandPayload& out_command) {
  return DecodeCommand(buffer, out_command);
}

}  // namespace protocol

#endif  // PROTOCOL_COMMAND_H_
