#include "protocol/join.h"

#include <algorithm>

namespace protocol {

bool EncodeJoinRequest(const JoinRequestPayload& payload,
                       engine::net::PacketBuffer& buffer) {
  const std::size_t name_len =
      std::min(payload.player_name.size(), kMaxPlayerNameLength);
  const std::size_t room_len =
      std::min(payload.room_code.size(), kMaxRoomCodeLength);

  if (name_len > 255 || room_len > 255) {
    return false;
  }
  buffer.WriteUint16(payload.client_version);
  buffer.WriteUint8(static_cast<std::uint8_t>(name_len));
  if (name_len > 0) {
    buffer.write_bytes(payload.player_name.data(), name_len);
  }
  buffer.WriteUint8(static_cast<std::uint8_t>(room_len));
  if (room_len > 0) {
    buffer.write_bytes(payload.room_code.data(), room_len);
  }
  return true;
}

bool DecodeJoinRequest(engine::net::PacketBuffer& buffer,
                       JoinRequestPayload& out_payload) {
  std::uint16_t client_version = 0;
  std::uint8_t name_len = 0;
  std::uint8_t room_len = 0;

  if (!buffer.ReadUint16(client_version)) {
    return false;
  }
  if (!buffer.ReadUint8(name_len)) {
    return false;
  }

  if (name_len > kMaxPlayerNameLength) {
    return false;
  }

  std::string player_name;
  if (name_len > 0) {
    player_name.resize(name_len);
    if (!buffer.read_bytes(player_name.data(), name_len)) {
      return false;
    }
  }

  if (!buffer.ReadUint8(room_len)) {
    return false;
  }

  if (room_len > kMaxRoomCodeLength) {
    return false;
  }

  std::string room_code;
  if (room_len > 0) {
    room_code.resize(room_len);
    if (!buffer.read_bytes(room_code.data(), room_len)) {
      return false;
    }
  }
  out_payload.client_version = client_version;
  out_payload.player_name = std::move(player_name);
  out_payload.room_code = std::move(room_code);
  return true;
}

bool EncodeJoinAccept(const JoinAcceptPayload& payload,
                      engine::net::PacketBuffer& buffer) {
  buffer.WriteUint16(payload.server_version);
  buffer.WriteUint32(payload.player_id);
  buffer.WriteUint8(payload.max_players);
  buffer.WriteUint8(payload.tick_rate);
  buffer.WriteUint32(payload.seed);
  return true;
}

bool DecodeJoinAccept(engine::net::PacketBuffer& buffer,
                      JoinAcceptPayload& out_payload) {
  std::uint16_t server_version = 0;
  std::uint32_t player_id = 0;
  std::uint8_t max_players = 0;
  std::uint8_t tick_rate = 0;
  std::uint32_t seed = 0;

  if (!buffer.ReadUint16(server_version) || !buffer.ReadUint32(player_id) ||
      !buffer.ReadUint8(max_players) || !buffer.ReadUint8(tick_rate) ||
      !buffer.ReadUint32(seed)) {
    return false;
  }
  out_payload.server_version = server_version;
  out_payload.player_id = player_id;
  out_payload.max_players = max_players;
  out_payload.tick_rate = tick_rate;
  out_payload.seed = seed;
  return true;
}

bool EncodeJoinReject(const JoinRejectPayload& payload,
                      engine::net::PacketBuffer& buffer) {
  const std::size_t msg_len =
      std::min(payload.message.size(), kMaxRejectMessageLength);

  if (msg_len > 255) {
    return false;
  }
  buffer.WriteUint16(payload.server_version);
  buffer.WriteUint8(static_cast<std::uint8_t>(payload.reason));
  buffer.WriteUint8(static_cast<std::uint8_t>(msg_len));
  if (msg_len > 0) {
    buffer.write_bytes(payload.message.data(), msg_len);
  }
  return true;
}

bool DecodeJoinReject(engine::net::PacketBuffer& buffer,
                      JoinRejectPayload& out_payload) {
  std::uint16_t server_version = 0;
  std::uint8_t reason_code = 0;
  std::uint8_t msg_len = 0;

  if (!buffer.ReadUint16(server_version) || !buffer.ReadUint8(reason_code) ||
      !buffer.ReadUint8(msg_len)) {
    return false;
  }

  if (msg_len > kMaxRejectMessageLength) {
    return false;
  }

  std::string message;
  if (msg_len > 0) {
    message.resize(msg_len);
    if (!buffer.read_bytes(message.data(), msg_len)) {
      return false;
    }
  }
  out_payload.server_version = server_version;
  out_payload.reason = static_cast<JoinRejectReason>(reason_code);
  out_payload.message = std::move(message);
  return true;
}
}  // namespace protocol
