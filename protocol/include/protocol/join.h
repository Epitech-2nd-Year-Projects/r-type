#ifndef PROTOCOL_JOIN_H
#define PROTOCOL_JOIN_H

#include <cstdint>
#include "engine/net/packet_buffer.h"


namespace protocol {

  inline constexpr std::size_t kMaxPlayerNameLength = 31;      ///< Maximum length for player name in bytes.
  inline constexpr std::size_t kMaxRoomCodeLength = 15;        ///< Maximum length for room code in bytes.
  inline constexpr std::size_t kMaxRejectMessageLength = 63;   ///< Maximum length for reject message in bytes.

  /**
   * @brief Reason codes for join rejection.
   */
  enum class JoinRejectReason : std::uint8_t {
    kUnknown = 0,           ///< Unknown reason.
    kVersionMismatch = 1,    ///< The client version does not match the server version.
    kServerFull = 2,        ///< The server is full and cannot accept more players.
    kInvalidRoom = 3,        ///< The requested game room does not exist.
    kBanned = 4,            ///< The client is banned from the server.
  };

  /**
   * @brief Payload for a client join request message.
   * 
   * Sent by the client when attempting to join a game room.
   */
  struct JoinRequestPayload {
    std::uint16_t client_version = 0;  ///< Client protocol version.
    std::string player_name;           ///< UTF-8, 0..kMaxPlayerNameLength bytes.
    std::string room_code;             ///< UTF-8, 0..kMaxRoomCodeLength bytes.
    std::string room_password;         ///< Optional private room password (4 digits).
  };

  /**
   * @brief Payload for a server join accept message.
   * 
   * Sent by the server when accepting a client's join request.
   */
  struct JoinAcceptPayload {
    std::uint16_t server_version = 0;  ///< Server protocol version.
    std::uint32_t player_id = 0;       ///< Assigned player ID.
    std::uint8_t max_players = 0;      ///< Maximum number of players in the room.
    std::uint8_t tick_rate = 0;        ///< Simulation steps per second.
    std::uint32_t seed = 0;            ///< Random seed for deterministic systems.
  };

  /**
   * @brief Payload for a server join reject message.
   * 
   * Sent by the server when rejecting a client's join request.
   */
  struct JoinRejectPayload {
    std::uint16_t server_version = 0;                   ///< Server protocol version.
    JoinRejectReason reason = JoinRejectReason::kUnknown;  ///< Reason for rejection.
    std::string message;                                ///< UTF-8, 0..kMaxRejectMessageLength bytes.
  };

  /**
   * @brief Serializes a JoinRequestPayload into a PacketBuffer.
   * @param request The join request to serialize.
   * @param writer The packet buffer to write to.
   * @return true on success, false if the writer runs out of space.
   */
  bool EncodeJoinRequest(const JoinRequestPayload& request, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes a JoinRequestPayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_request Output parameter for the deserialized join request.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodeJoinRequest(engine::net::PacketBuffer& reader, JoinRequestPayload& out_request);

  /**
   * @brief Serializes a JoinAcceptPayload into a PacketBuffer.
   * @param accept The join accept to serialize.
   * @param writer The packet buffer to write to.
   */
  bool EncodeJoinAccept(const JoinAcceptPayload& accept, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes a JoinAcceptPayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_accept Output parameter for the deserialized join accept.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodeJoinAccept(engine::net::PacketBuffer& reader, JoinAcceptPayload& out_accept);

  /**
   * @brief Serializes a JoinRejectPayload into a PacketBuffer.
   * @param reject The join reject to serialize.
   * @param writer The packet buffer to write to.
   * @return true on success, false if the writer runs out of space.
   */
  bool EncodeJoinReject(const JoinRejectPayload& reject, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes a JoinRejectPayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_reject Output parameter for the deserialized join reject.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodeJoinReject(engine::net::PacketBuffer& reader, JoinRejectPayload& out_reject);

}

#endif // !PROTOCOL_JOIN_H_
