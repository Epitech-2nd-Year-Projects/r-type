#include "protocol/command.h"

namespace protocol {

bool EncodeCommand(const CommandPayload& payload,
                   engine::net::PacketBuffer& buffer) {
  buffer.WriteUint16(payload.command_id);
  if (!buffer.WriteString(payload.payload)) {
    return false;
  }
  return true;
}

bool DecodeCommand(engine::net::PacketBuffer& buffer,
                   CommandPayload& out_payload) {
  std::uint16_t command_id = 0;
  std::string payload;

  if (!buffer.ReadUint16(command_id)) {
    return false;
  }
  if (!buffer.ReadString(payload)) {
    return false;
  }
  out_payload.command_id = command_id;
  out_payload.payload = std::move(payload);
  return true;
}

}  // namespace protocol
