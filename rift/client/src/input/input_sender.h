#ifndef RIFT_CLIENT_INPUT_SENDER_H_
#define RIFT_CLIENT_INPUT_SENDER_H_

#include <cstdint>

#include "engine/time/time_delta.h"
#include "input/input_buffer.h"
#include "protocol/input_state.h"

namespace rift::client {

class WorldUpdateReceiver;
class FightInputLayer;

class InputSender {
 public:
  InputSender(FightInputLayer& input_layer, WorldUpdateReceiver& receiver);

  void Reset();

  void SetSendRateHz(float hz);

  void Update(engine::time::TimeDelta dt, bool sending_enabled);

 private:
  protocol::InputCommand BuildCommand(const BufferedInputSample& sample);
  bool SendPayload(const protocol::InputStatePayload& payload,
                   std::uint32_t client_time_ms);

  FightInputLayer& input_layer_;
  WorldUpdateReceiver& receiver_;
  InputBuffer input_buffer_{};
  protocol::InputHistoryWindow history_{};
  std::uint32_t next_input_sequence_{1};
  float send_rate_hz_{60.0f};
  float send_interval_seconds_{1.0f / 60.0f};
  float accumulator_seconds_{0.0f};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_INPUT_SENDER_H_
