#include "protocol/player_died.h"

namespace protocol {

bool EncodePlayerDied(const PlayerDiedPayload& payload,
                      engine::net::PacketBuffer& buffer) {
  buffer.WriteUint32(payload.player_id);
  buffer.WriteUint32(payload.killer_entity_id);
  buffer.WriteUint8(static_cast<std::uint8_t>(payload.cause));
  buffer.WriteUint8(payload.remaining_lives);
  return true;
}

bool DecodePlayerDied(engine::net::PacketBuffer& buffer,
                      PlayerDiedPayload& out_payload) {
  std::uint32_t player_id = 0;
  std::uint32_t killer_entity_id = 0;
  std::uint8_t cause_code = 0;
  std::uint8_t remaining_lives = 0;

  if (!buffer.ReadUint32(player_id) || !buffer.ReadUint32(killer_entity_id) ||
      !buffer.ReadUint8(cause_code) || !buffer.ReadUint8(remaining_lives)) {
    return false;
  }
  out_payload.player_id = player_id;
  out_payload.killer_entity_id = killer_entity_id;
  out_payload.cause = static_cast<DeathCause>(cause_code);
  out_payload.remaining_lives = remaining_lives;
  return true;
}
}  // namespace protocol
