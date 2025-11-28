#include "protocol/input_state.h"

namespace protocol {

bool EncodeInputState(const InputStatePayload& input,
                      engine::net::PacketBuffer& writer) {
  writer.WriteUint32(input.input_sequence);
  writer.WriteUint8(input.buttons);
  writer.WriteInt16(input.analog_x);
  writer.WriteInt16(input.analog_y);
  writer.WriteUint32(input.client_time_ms);
  return true;
}

bool DecodeInputState(engine::net::PacketBuffer& reader,
                      InputStatePayload& out_input) {
  std::uint32_t input_sequence;
  std::uint8_t buttons;
  std::int16_t analog_x;
  std::int16_t analog_y;
  std::uint32_t client_time_ms;

  if (!reader.ReadUint32(input_sequence) || !reader.ReadUint8(buttons) ||
      !reader.ReadInt16(analog_x) || !reader.ReadInt16(analog_y) ||
      !reader.ReadUint32(client_time_ms)) {
    return false;
  }
  out_input.input_sequence = input_sequence;
  out_input.buttons = buttons;
  out_input.analog_x = analog_x;
  out_input.analog_y = analog_y;
  out_input.client_time_ms = client_time_ms;
  return true;
}
}  // namespace protocol
