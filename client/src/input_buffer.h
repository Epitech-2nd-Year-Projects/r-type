/**
 * @file input_buffer.h
 * @brief Buffers gameplay input transitions before network replication
 */

#ifndef CLIENT_INPUT_BUFFER_H_
#define CLIENT_INPUT_BUFFER_H_

#include <cstdint>
#include <deque>
#include <vector>

#include "input_layer.h"

namespace client {

/**
 * @brief Snapshot of gameplay input captured at a point in time
 */
struct BufferedInputSample {
  ActionState state{};
  std::uint32_t client_time_ms{0};
};

/**
 * @class InputBuffer
 * @brief Collects raw input transitions and exposes a deduplicated stream
 *
 * @details
 * Maintains the latest gameplay input state and records transitions in order,
 * ensuring short-lived changes are preserved even when the network send rate
 * is lower than the input polling rate. Consecutive identical states are
 * deduplicated to keep the outgoing queue compact.
 */
class InputBuffer {
 public:
  /**
   * @brief Reset the buffer to a known state
   * @param initial_state Gameplay input state to seed the buffer
   * @param time_ms Timestamp used for the seeded sample
   */
  void Reset(const ActionState& initial_state, std::uint32_t time_ms);

  /**
   * @brief Apply raw action events and enqueue resulting states
   * @param events Ordered gameplay events captured during the frame
   * @param time_ms Timestamp assigned to each enqueued transition
   */
  void PushEvents(const std::vector<GameActionEvent>& events,
                  std::uint32_t time_ms);

  /**
   * @brief Retrieve the next input sample to replicate
   * @param fallback_time_ms Timestamp used when no pending transitions exist
   * @return Buffered input sample representing the next state to send
   */
  BufferedInputSample NextSample(std::uint32_t fallback_time_ms);

  /**
   * @brief Accessor for the current gameplay input state
   */
  const ActionState& state() const { return current_state_; }

 private:
  bool ApplyEvent(const GameActionEvent& event);
  void EnqueueCurrent(std::uint32_t time_ms);
  bool MatchesLastQueued(const ActionState& state) const;

  ActionState current_state_{};
  ActionState last_queued_state_{};
  std::deque<BufferedInputSample> pending_samples_{};
};

}  // namespace client

#endif  // CLIENT_INPUT_BUFFER_H_
