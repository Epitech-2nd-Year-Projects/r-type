#include "protocol/gameplay_ping.h"

namespace protocol {

bool EncodeGameplayPing(const GameplayPingPayload& payload,
                         engine::net::PacketBuffer& writer) {
  writer.WriteUint32(payload.sender_id);
  writer.WriteUint8(static_cast<std::uint8_t>(payload.type));
  writer.WriteFloat(payload.x);
  writer.WriteFloat(payload.y);
  return true;
}

bool DecodeGameplayPing(engine::net::PacketBuffer& reader,
                         GameplayPingPayload& out_payload) {
  if (!reader.ReadUint32(out_payload.sender_id)) return false;
  std::uint8_t type_val = 0;
  if (!reader.ReadUint8(type_val)) return false;
  out_payload.type = static_cast<PingType>(type_val);
  if (!reader.ReadFloat(out_payload.x)) return false;
  if (!reader.ReadFloat(out_payload.y)) return false;
  return true;
}

} // namespace protocol
