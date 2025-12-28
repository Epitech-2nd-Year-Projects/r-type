#include "input/input_sender.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "engine/time/monotonic_time.h"
#include "input/input_layer.h"
#include "logging.h"
#include "world_update_receiver.h"

namespace {

std::uint32_t NowMilliseconds() {
  // Protocol header uses 32-bit millisecond timestamps; wraparound is accepted
  // by the server for latency measurement.
  return static_cast<std::uint32_t>(engine::time::NowMilliseconds());
}

constexpr int kMaxBurstPerUpdate = 4;

std::uint8_t BuildButtonMask(const client::ActionState& state) {
  std::uint8_t buttons = 0;
  if (state.move_up) buttons |= protocol::kInputUp;
  if (state.move_down) buttons |= protocol::kInputDown;
  if (state.move_left) buttons |= protocol::kInputLeft;
  if (state.move_right) buttons |= protocol::kInputRight;
  if (state.shoot) buttons |= protocol::kInputFire;
  if (state.big_shoot) buttons |= protocol::kInputBigFire;
  return buttons;
}

}  // namespace

namespace client {

InputSender::InputSender(InputLayer& input_layer, WorldUpdateReceiver& receiver)
    : input_layer_(input_layer), receiver_(receiver) {
  input_buffer_.Reset(input_layer_.state(), NowMilliseconds());
}

void InputSender::Reset() {
  input_buffer_.Reset(input_layer_.state(), NowMilliseconds());
  history_ = protocol::InputHistoryWindow{};
  next_input_sequence_ = 1;
  accumulator_seconds_ = 0.0f;
}

void InputSender::SetSendRateHz(float hz) {
  const float clamped =
      std::clamp(hz, 30.0f, 120.0f);  // Avoid starving or flooding the network.
  send_rate_hz_ = clamped;
  send_interval_seconds_ = 1.0f / send_rate_hz_;
  accumulator_seconds_ = std::min(accumulator_seconds_, send_interval_seconds_);
}

void InputSender::Update(engine::time::TimeDelta dt, bool sending_enabled) {
  if (!sending_enabled || !receiver_.running()) {
    return;
  }

  const auto frame_time_ms = NowMilliseconds();
  input_buffer_.PushEvents(input_layer_.ConsumeEvents(), frame_time_ms);

  accumulator_seconds_ += dt.as_seconds();

  int sends_this_update = 0;
  while (accumulator_seconds_ >= send_interval_seconds_ &&
         sends_this_update < kMaxBurstPerUpdate) {
    accumulator_seconds_ -= send_interval_seconds_;
    const auto sample = input_buffer_.NextSample(NowMilliseconds());
    const auto command = BuildCommand(sample);
    history_.Push(command);
    const auto payload = history_.BuildPayload();
    if (!SendPayload(payload, command.client_time_ms)) {
      LogPacketError("input send", "failed to encode or queue InputState");
      break;
    }
    ++sends_this_update;
  }

  const float max_accumulator =
      send_interval_seconds_ * static_cast<float>(kMaxBurstPerUpdate);
  if (accumulator_seconds_ > max_accumulator) {
    accumulator_seconds_ = std::min(accumulator_seconds_, max_accumulator);
  }
}

protocol::InputCommand InputSender::BuildCommand(
    const BufferedInputSample& sample) {
  protocol::InputCommand command{};
  command.input_sequence = next_input_sequence_++;

  command.buttons = BuildButtonMask(sample.state);
  command.analog_x = static_cast<std::int16_t>(
      (sample.state.move_right ? 1 : 0) - (sample.state.move_left ? 1 : 0));
  command.analog_y = static_cast<std::int16_t>(
      (sample.state.move_down ? 1 : 0) - (sample.state.move_up ? 1 : 0));
  command.client_time_ms = sample.client_time_ms;
  return command;
}

bool InputSender::SendPayload(const protocol::InputStatePayload& payload,
                              std::uint32_t client_time_ms) {
  return receiver_.EnqueueInputState(payload, client_time_ms);
}

}  // namespace client
