#include "protocol/input_state.h"

namespace protocol {

void EncodeInputState(const InputStatePlayload& input, engine::net::PacketBuffer& writer) {
  writer.WriteUint32(input.input_sequence);
  writer.WriteUint8(input.buttons);
  writer.WriteUint16(input.analog_x);
  writer.WriteUint16(input.analog_y);
  writer.WriteUint32(input.timestamp_ms);
}

bool DecodeInputState(engine::net::PacketBuffer& reader, InputStatePlayload& out_input) {
  std::uint32_t input_sequence;
  std::uint8_t buttons;
  std::uint16_t analog_x;
  std::uint16_t analog_y;
  std::uint32_t timestamp_ms;

  if (!reader.ReadUint32(input_sequence) ||
      !reader.ReadUint8(buttons) ||
      !reader.ReadUint16(analog_x) ||
      !reader.ReadUint16(analog_y) ||
      !reader.ReadUint32(timestamp_ms)) {
    return false;
  }
  out_input.input_sequence = input_sequence;
  out_input.buttons = buttons;
  out_input.analog_x = analog_x;
  out_input.analog_y = analog_y;
  out_input.timestamp_ms = timestamp_ms;
  return true;
}
}