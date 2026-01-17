#ifndef RIFT_CLIENT_INPUT_BUFFER_H_
#define RIFT_CLIENT_INPUT_BUFFER_H_

#include <cstdint>
#include <deque>
#include <vector>

#include "input/fight_input.h"

namespace rift::client {

struct BufferedInputSample {
  FightActionState state{};
  std::uint32_t client_time_ms{0};
};

class InputBuffer {
 public:
  void Reset(const FightActionState& initial_state, std::uint32_t time_ms);

  void PushEvents(const std::vector<FightActionEvent>& events,
                  std::uint32_t time_ms);

  BufferedInputSample NextSample(std::uint32_t fallback_time_ms);

  const FightActionState& state() const { return current_state_; }

 private:
  bool ApplyEvent(const FightActionEvent& event);
  void EnqueueCurrent(std::uint32_t time_ms);
  bool MatchesLastQueued(const FightActionState& state) const;

  FightActionState current_state_{};
  FightActionState last_queued_state_{};
  std::deque<BufferedInputSample> pending_samples_{};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_INPUT_BUFFER_H_
