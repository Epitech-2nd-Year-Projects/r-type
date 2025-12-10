#include "input_buffer.h"

namespace client {
namespace {

bool StatesEqual(const ActionState& lhs, const ActionState& rhs) {
  return lhs.move_up == rhs.move_up && lhs.move_down == rhs.move_down &&
         lhs.move_left == rhs.move_left && lhs.move_right == rhs.move_right &&
         lhs.shoot == rhs.shoot;
}

}  // namespace

void InputBuffer::Reset(const ActionState& initial_state,
                        std::uint32_t time_ms) {
  current_state_ = initial_state;
  last_queued_state_ = initial_state;
  pending_samples_.clear();
  pending_samples_.push_back({initial_state, time_ms});
}

void InputBuffer::PushEvents(const std::vector<GameActionEvent>& events,
                             std::uint32_t time_ms) {
  for (const auto& event : events) {
    if (ApplyEvent(event)) {
      EnqueueCurrent(time_ms);
    }
  }
}

BufferedInputSample InputBuffer::NextSample(std::uint32_t fallback_time_ms) {
  if (!pending_samples_.empty()) {
    auto sample = pending_samples_.front();
    pending_samples_.pop_front();
    return sample;
  }
  return {current_state_, fallback_time_ms};
}

bool InputBuffer::ApplyEvent(const GameActionEvent& event) {
  bool* target = nullptr;
  switch (event.action) {
    case GameAction::kMoveUp:
      target = &current_state_.move_up;
      break;
    case GameAction::kMoveDown:
      target = &current_state_.move_down;
      break;
    case GameAction::kMoveLeft:
      target = &current_state_.move_left;
      break;
    case GameAction::kMoveRight:
      target = &current_state_.move_right;
      break;
    case GameAction::kShoot:
      target = &current_state_.shoot;
      break;
    case GameAction::kReconnect:
      return false;
  }

  const bool pressed = event.type == GameActionEventType::kPressed;
  if (*target == pressed) {
    return false;
  }
  *target = pressed;
  return true;
}

void InputBuffer::EnqueueCurrent(std::uint32_t time_ms) {
  if (MatchesLastQueued(current_state_)) {
    return;
  }
  pending_samples_.push_back({current_state_, time_ms});
  last_queued_state_ = current_state_;
}

bool InputBuffer::MatchesLastQueued(const ActionState& state) const {
  if (!pending_samples_.empty()) {
    return StatesEqual(pending_samples_.back().state, state);
  }
  return StatesEqual(last_queued_state_, state);
}

}  // namespace client
