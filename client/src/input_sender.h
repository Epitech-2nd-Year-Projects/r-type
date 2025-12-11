/**
 * @file input_sender.h
 * @brief Periodic input replication to the server
 */

#ifndef CLIENT_INPUT_SENDER_H_
#define CLIENT_INPUT_SENDER_H_

#include <cstdint>
#include "engine/time/time_delta.h"
#include "input_buffer.h"
#include "protocol/input_state.h"

namespace client {

class WorldUpdateReceiver;

class InputLayer;

/**
 * @class InputSender
 * @brief Builds InputState payloads and emits them over UDP
 *
 * @details
 * Samples the gameplay action state maintains a rolling redundancy window and
 * tags packets with protocol timestamps and sequence information
 */
class InputSender {
 public:
  /**
   * @brief Construct an input sender bound to the input layer and network worker
   */
  InputSender(InputLayer& input_layer, WorldUpdateReceiver& receiver);

  /**
   * @brief Reset input history and local sequence counters
   */
  void Reset();

  /**
   * @brief Configure the desired input send cadence
   * @param hz Target send rate in Hz clamped to a safe range
   */
  void SetSendRateHz(float hz);

  /**
   * @brief Pump input replication at a fixed cadence when enabled
   */
  void Update(engine::time::TimeDelta dt, bool sending_enabled);

 private:
  protocol::InputCommand BuildCommand(const BufferedInputSample& sample);
  bool SendPayload(const protocol::InputStatePayload& payload,
                   std::uint32_t client_time_ms);

  InputLayer& input_layer_;
  WorldUpdateReceiver& receiver_;
  InputBuffer input_buffer_{};
  protocol::InputHistoryWindow history_{};
  std::uint32_t next_input_sequence_{1};
  float send_rate_hz_{60.0f};
  float send_interval_seconds_{1.0f / 60.0f};
  float accumulator_seconds_{0.0f};
};

}  // namespace client

#endif  // CLIENT_INPUT_SENDER_H_
