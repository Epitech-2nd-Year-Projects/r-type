#include "input_sender.h"

#include <chrono>

#include "engine/time/monotonic_time.h"
#include "input_layer.h"
#include "logging.h"
#include "world_update_receiver.h"

namespace {

std::uint32_t NowMilliseconds() {
  // Protocol header uses 32-bit millisecond timestamps; wraparound is accepted
  // by the server for latency measurement.
  return static_cast<std::uint32_t>(engine::time::NowMilliseconds());
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

InputSender::InputSender(InputLayer& input_layer, WorldUpdateReceiver& receiver)
    : input_layer_(input_layer), receiver_(receiver) {}

void InputSender::Reset() {
  history_ = protocol::InputHistoryWindow{};
  next_input_sequence_ = 1;
  accumulator_seconds_ = 0.0f;
}

void InputSender::Update(engine::time::TimeDelta dt, bool sending_enabled) {
  if (!sending_enabled || !receiver_.running()) {
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
  return receiver_.EnqueueInputState(payload, client_time_ms);
}

}  // namespace client
