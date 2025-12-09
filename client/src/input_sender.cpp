#include "input_sender.h"

#include <chrono>

#include "input_layer.h"
#include "logging.h"
#include "network_transport.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/sequence_tracker.h"

namespace {

std::uint32_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(now).count());
}

std::uint8_t BuildButtonMask(const client::ActionState& state) {
  std::uint8_t buttons = 0;
  if (state.move_up) buttons |= protocol::kInputUp;
  if (state.move_down) buttons |= protocol::kInputDown;
  if (state.move_left) buttons |= protocol::kInputLeft;
  if (state.move_right) buttons |= protocol::kInputRight;
  if (state.shoot) buttons |= protocol::kInputFire;
  return buttons;
}

}  // namespace

namespace client {

InputSender::InputSender(InputLayer& input_layer, NetworkTransport& transport,
                         protocol::SequenceTracker& sequence_tracker)
    : input_layer_(input_layer),
      transport_(transport),
      sequence_tracker_(sequence_tracker) {}

void InputSender::Reset() {
  history_ = protocol::InputHistoryWindow{};
  next_input_sequence_ = 1;
  accumulator_seconds_ = 0.0f;
}

void InputSender::Update(engine::time::TimeDelta dt, bool sending_enabled) {
  if (!sending_enabled || !transport_.running()) {
    return;
  }

  accumulator_seconds_ += dt.as_seconds();

  while (accumulator_seconds_ >= send_interval_seconds_) {
    accumulator_seconds_ -= send_interval_seconds_;
    const auto command = BuildCommand();
    history_.Push(command);
    const auto payload = history_.BuildPayload();
    if (!SendPayload(payload, command.client_time_ms)) {
      LogPacketError("input send", "failed to encode or queue InputState");
      break;
    }
  }
}

protocol::InputCommand InputSender::BuildCommand() {
  protocol::InputCommand command{};
  command.input_sequence = next_input_sequence_++;

  const ActionState state = input_layer_.state();
  command.buttons = BuildButtonMask(state);
  command.analog_x = static_cast<std::int16_t>((state.move_right ? 1 : 0) -
                                               (state.move_left ? 1 : 0));
  command.analog_y = static_cast<std::int16_t>((state.move_down ? 1 : 0) -
                                               (state.move_up ? 1 : 0));
  command.client_time_ms = NowMilliseconds();
  return command;
}

bool InputSender::SendPayload(const protocol::InputStatePayload& payload,
                              std::uint32_t client_time_ms) {
  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type = static_cast<std::uint8_t>(
      protocol::message_type::MessageType::kInputState);
  packet.header.flags = 0;
  packet.header.sequence = sequence_tracker_.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = client_time_ms;
  sequence_tracker_.FillAckFields(&packet.header);
  packet.payload = payload;

  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet, buffer)) {
    return false;
  }
  return transport_.Send(std::move(buffer));
}

}  // namespace client
