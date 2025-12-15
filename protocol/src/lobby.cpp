#include "protocol/lobby.h"

#include <algorithm>
#include <limits>
#include <span>

namespace protocol {

namespace {

constexpr std::uint8_t BoolToByte(bool value) { return value ? 1 : 0; }

bool TryReadBool(engine::net::PacketBuffer& buffer, bool& out_value) {
  std::uint8_t value = 0;
  if (!buffer.ReadUint8(value)) {
    return false;
  }
  out_value = value != 0;
  return true;
}

bool EncodeStringWithLength(std::string_view value,
                            std::size_t max_length,
                            engine::net::PacketBuffer& buffer) {
  const std::size_t len = std::min(value.size(), max_length);
  if (len > std::numeric_limits<std::uint8_t>::max()) {
    return false;
  }
  buffer.WriteUint8(static_cast<std::uint8_t>(len));
  if (len > 0) {
    buffer.write_bytes(std::as_bytes(std::span(value).first(len)));
  }
  return true;
}

bool DecodeStringWithLength(engine::net::PacketBuffer& buffer,
                            std::size_t max_length,
                            std::string& out_value) {
  std::uint8_t len = 0;
  if (!buffer.ReadUint8(len)) {
    return false;
  }
  if (len > max_length) {
    return false;
  }
  std::string value;
  if (len > 0) {
    value.resize(len);
    if (!buffer.read_bytes(std::as_writable_bytes(std::span(value).first(len)))) {
      return false;
    }
  }
  out_value = std::move(value);
  return true;
}

}  // namespace

bool EncodeRoomSummary(const RoomSummary& summary,
                       engine::net::PacketBuffer& buffer) {
  if (!EncodeStringWithLength(summary.room_code, kMaxRoomCodeLength, buffer)) {
    return false;
  }
  buffer.WriteUint8(BoolToByte(summary.is_private));
  buffer.WriteUint8(summary.player_count);
  buffer.WriteUint8(summary.max_players);
  return true;
}

bool DecodeRoomSummary(engine::net::PacketBuffer& buffer,
                       RoomSummary& out_summary) {
  RoomSummary summary{};
  if (!DecodeStringWithLength(buffer, kMaxRoomCodeLength, summary.room_code)) {
    return false;
  }
  if (summary.room_code.empty()) {
    return false;
  }
  bool is_private = false;
  if (!TryReadBool(buffer, is_private)) {
    return false;
  }
  std::uint8_t player_count = 0;
  std::uint8_t max_players = 0;
  if (!buffer.ReadUint8(player_count) || !buffer.ReadUint8(max_players)) {
    return false;
  }
  summary.is_private = is_private;
  summary.player_count = player_count;
  summary.max_players = max_players;
  out_summary = std::move(summary);
  return true;
}

bool EncodeRoomListRequest(const RoomListRequestPayload& request,
                           engine::net::PacketBuffer& buffer) {
  (void)request;
  (void)buffer;
  return true;
}

bool DecodeRoomListRequest(engine::net::PacketBuffer& buffer,
                           RoomListRequestPayload& out_request) {
  (void)buffer;
  out_request = RoomListRequestPayload{};
  return true;
}

bool EncodeRoomListResponse(const RoomListResponsePayload& response,
                            engine::net::PacketBuffer& buffer) {
  const std::size_t count = std::min(response.rooms.size(), kMaxRoomListEntries);
  buffer.WriteUint8(static_cast<std::uint8_t>(count));
  for (std::size_t i = 0; i < count; ++i) {
    if (!EncodeRoomSummary(response.rooms[i], buffer)) {
      return false;
    }
  }
  return true;
}

bool DecodeRoomListResponse(engine::net::PacketBuffer& buffer,
                            RoomListResponsePayload& out_response) {
  std::uint8_t count = 0;
  if (!buffer.ReadUint8(count)) {
    return false;
  }
  if (count > kMaxRoomListEntries) {
    return false;
  }
  RoomListResponsePayload response{};
  response.rooms.reserve(count);
  for (std::uint8_t i = 0; i < count; ++i) {
    RoomSummary summary{};
    if (!DecodeRoomSummary(buffer, summary)) {
      return false;
    }
    response.rooms.push_back(std::move(summary));
  }
  out_response = std::move(response);
  return true;
}

bool EncodeCreateRoomRequest(const CreateRoomRequestPayload& request,
                             engine::net::PacketBuffer& buffer) {
  if (!EncodeStringWithLength(request.room_code, kMaxRoomCodeLength, buffer)) {
    return false;
  }
  buffer.WriteUint8(BoolToByte(request.is_private));
  buffer.WriteUint8(request.max_players);
  return true;
}

bool DecodeCreateRoomRequest(engine::net::PacketBuffer& buffer,
                             CreateRoomRequestPayload& out_request) {
  CreateRoomRequestPayload request{};
  if (!DecodeStringWithLength(buffer, kMaxRoomCodeLength, request.room_code)) {
    return false;
  }
  bool is_private = false;
  if (!TryReadBool(buffer, is_private)) {
    return false;
  }
  std::uint8_t max_players = 0;
  if (!buffer.ReadUint8(max_players)) {
    return false;
  }
  request.is_private = is_private;
  request.max_players = max_players;
  out_request = std::move(request);
  return true;
}

bool EncodeCreateRoomResponse(const CreateRoomResponsePayload& response,
                              engine::net::PacketBuffer& buffer) {
  buffer.WriteUint8(BoolToByte(response.success));
  if (!EncodeStringWithLength(response.message, kMaxRoomMessageLength, buffer)) {
    return false;
  }
  const bool has_room = response.room.has_value();
  buffer.WriteUint8(BoolToByte(has_room));
  if (has_room) {
    if (!EncodeRoomSummary(*response.room, buffer)) {
      return false;
    }
  }
  return true;
}

bool DecodeCreateRoomResponse(engine::net::PacketBuffer& buffer,
                              CreateRoomResponsePayload& out_response) {
  CreateRoomResponsePayload response{};
  bool success = false;
  if (!TryReadBool(buffer, success)) {
    return false;
  }
  response.success = success;
  if (!DecodeStringWithLength(buffer, kMaxRoomMessageLength, response.message)) {
    return false;
  }
  bool has_room = false;
  if (!TryReadBool(buffer, has_room)) {
    return false;
  }
  if (has_room) {
    RoomSummary summary{};
    if (!DecodeRoomSummary(buffer, summary)) {
      return false;
    }
    response.room = std::move(summary);
  }
  out_response = std::move(response);
  return true;
}

}  // namespace protocol
