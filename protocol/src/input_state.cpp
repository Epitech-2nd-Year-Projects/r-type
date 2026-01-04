#include "protocol/input_state.h"

namespace protocol {

InputHistoryWindow::InputHistoryWindow() : window_{}, count_{0} {};

void InputHistoryWindow::Push(const InputCommand& command) {
  if (count_ > 0) {
    const std::size_t limit = count_ < kMaxInputSequenceHistory
                                  ? count_
                                  : kMaxInputSequenceHistory - 1;
    for (std::size_t i = limit; i > 0; --i) {
      window_[i] = window_[i - 1];
    }
  }
  window_[0] = command;
  if (count_ < kMaxInputSequenceHistory) {
    ++count_;
  }
}

InputStatePayload InputHistoryWindow::BuildPayload() const {
  InputStatePayload payload{};
  payload.command_count = static_cast<std::uint8_t>(count_);
  for (std::size_t i = 0; i < count_; ++i) {
    payload.commands[i] = window_[i];
  }
  return payload;
}

bool EncodeInputState(const InputStatePayload& input,
                      engine::net::PacketBuffer& writer) {
  std::uint8_t command_count = input.command_count;

  if (command_count == 0 || command_count > kMaxInputSequenceHistory) {
    return false;
  }
  writer.WriteUint32(input.ack_snapshot_id);
  writer.WriteUint8(command_count);
  for (std::size_t i = 0; i < command_count; ++i) {
    const InputCommand& command = input.commands[i];
    writer.WriteUint32(command.input_sequence);
    writer.WriteUint8(command.buttons);
    writer.WriteInt16(command.analog_x);
    writer.WriteInt16(command.analog_y);
    writer.WriteUint32(command.client_time_ms);
  }
  return true;
}

bool DecodeInputState(engine::net::PacketBuffer& reader,
                      InputStatePayload& out_input) {
  std::uint8_t command_count = 0;

  std::uint32_t ack_snapshot_id = 0;
  if (!reader.ReadUint32(ack_snapshot_id) || !reader.ReadUint8(command_count)) {
    return false;
  }
  if (command_count == 0 || command_count > kMaxInputSequenceHistory) {
    return false;
  }
  InputStatePayload result;
  result.ack_snapshot_id = ack_snapshot_id;
  result.command_count = command_count;
  for (std::size_t i = 0; i < command_count; ++i) {
    InputCommand command;
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
    command.input_sequence = input_sequence;
    command.buttons = buttons;
    command.analog_x = analog_x;
    command.analog_y = analog_y;
    command.client_time_ms = client_time_ms;
    result.commands[i] = command;
  }
  out_input = result;
  return true;
}
}  // namespace protocol
