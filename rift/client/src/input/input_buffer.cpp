#include "input/input_buffer.h"

namespace rift::client {

void InputBuffer::Reset(const FightActionState& initial_state,
                        std::uint32_t time_ms) {
  current_state_ = initial_state;
  last_queued_state_ = initial_state;
  pending_samples_.clear();
  pending_samples_.push_back({initial_state, time_ms});
}

void InputBuffer::PushEvents(const std::vector<FightActionEvent>& events,
                             std::uint32_t time_ms) {
  for (const auto& event : events) {
    if (ApplyEvent(event)) {
      EnqueueCurrent(time_ms);
    }
  }
}

BufferedInputSample InputBuffer::NextSample(std::uint32_t fallback_time_ms) {
  if (pending_samples_.empty()) {
    return {current_state_, fallback_time_ms};
  }
  BufferedInputSample sample = pending_samples_.front();
  pending_samples_.pop_front();
  return sample;
}

bool InputBuffer::ApplyEvent(const FightActionEvent& event) {
  const bool pressed = event.type == FightActionEventType::kPressed;
  bool changed = false;

  switch (event.action) {
    case FightAction::kMoveLeft:
      if (current_state_.move_left != pressed) {
        current_state_.move_left = pressed;
        changed = true;
      }
      break;
    case FightAction::kMoveRight:
      if (current_state_.move_right != pressed) {
        current_state_.move_right = pressed;
        changed = true;
      }
      break;
    case FightAction::kLightAttack:
      if (current_state_.light_attack != pressed) {
        current_state_.light_attack = pressed;
        changed = true;
      }
      break;
    case FightAction::kHeavyAttack:
      if (current_state_.heavy_attack != pressed) {
        current_state_.heavy_attack = pressed;
        changed = true;
      }
      break;
    case FightAction::kBlock:
      if (current_state_.block != pressed) {
        current_state_.block = pressed;
        changed = true;
      }
      break;
    case FightAction::kDodge:
      if (current_state_.dodge != pressed) {
        current_state_.dodge = pressed;
        changed = true;
      }
      break;
  }

  return changed;
}

void InputBuffer::EnqueueCurrent(std::uint32_t time_ms) {
  if (MatchesLastQueued(current_state_)) {
    return;
  }
  pending_samples_.push_back({current_state_, time_ms});
  last_queued_state_ = current_state_;
}

bool InputBuffer::MatchesLastQueued(const FightActionState& state) const {
  return state.move_left == last_queued_state_.move_left &&
         state.move_right == last_queued_state_.move_right &&
         state.light_attack == last_queued_state_.light_attack &&
         state.heavy_attack == last_queued_state_.heavy_attack &&
         state.block == last_queued_state_.block &&
         state.dodge == last_queued_state_.dodge;
}

}  // namespace rift::client
