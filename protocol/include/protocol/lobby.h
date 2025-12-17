#ifndef PROTOCOL_LOBBY_H_
#define PROTOCOL_LOBBY_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/net/packet_buffer.h"
#include "protocol/join.h"

namespace protocol {

inline constexpr std::size_t kMaxRoomListEntries = 64;     ///< Maximum rooms returned in a single listing.
inline constexpr std::size_t kMaxRoomMessageLength = 63;   ///< Maximum length for lobby status messages.
inline constexpr std::size_t kMaxRoomNameLength = 31;      ///< Maximum length for room names.

/**
 * @brief Snapshot of a room exposed to clients.
 */
struct RoomSummary {
  std::string room_code;     ///< Unique room code used for joining (opaque to users).
  std::string room_name;     ///< Human-readable room name.
  std::uint8_t max_players;  ///< Configured capacity.
  std::uint8_t player_count; ///< Current occupancy.
  bool is_private;           ///< Whether the room is private (code required).
  bool started{false};       ///< Whether gameplay has begun (joins locked).
};

/**
 * @brief Empty payload used to request the current room directory.
 */
struct RoomListRequestPayload {};

/**
 * @brief List of rooms returned by the server.
 */
struct RoomListResponsePayload {
  std::vector<RoomSummary> rooms;
};

/**
 * @brief Request to create a new room on the server.
 */
struct CreateRoomRequestPayload {
  std::string room_name;   ///< Display name chosen by creator.
  std::uint8_t max_players{0};  ///< Requested capacity (1..255).
  bool is_private{false};  ///< Whether the room should be private with a 4-digit code.
  std::string room_password;  ///< Optional creator-specified password for private rooms.
};

/**
 * @brief Response indicating whether room creation succeeded.
 */
struct CreateRoomResponsePayload {
  bool success{false};
  std::string message;                          ///< Human-readable status.
  std::optional<RoomSummary> room;              ///< Created room (present on success).
  std::string room_password;                    ///< Populated for private rooms.
};

bool EncodeRoomSummary(const RoomSummary& summary,
                       engine::net::PacketBuffer& buffer);
bool DecodeRoomSummary(engine::net::PacketBuffer& buffer,
                       RoomSummary& out_summary);

bool EncodeRoomListRequest(const RoomListRequestPayload& request,
                           engine::net::PacketBuffer& buffer);
bool DecodeRoomListRequest(engine::net::PacketBuffer& buffer,
                           RoomListRequestPayload& out_request);

bool EncodeRoomListResponse(const RoomListResponsePayload& response,
                            engine::net::PacketBuffer& buffer);
bool DecodeRoomListResponse(engine::net::PacketBuffer& buffer,
                            RoomListResponsePayload& out_response);

bool EncodeCreateRoomRequest(const CreateRoomRequestPayload& request,
                             engine::net::PacketBuffer& buffer);
bool DecodeCreateRoomRequest(engine::net::PacketBuffer& buffer,
                             CreateRoomRequestPayload& out_request);

bool EncodeCreateRoomResponse(const CreateRoomResponsePayload& response,
                              engine::net::PacketBuffer& buffer);
bool DecodeCreateRoomResponse(engine::net::PacketBuffer& buffer,
                              CreateRoomResponsePayload& out_response);

}  // namespace protocol

#endif  // PROTOCOL_LOBBY_H_
