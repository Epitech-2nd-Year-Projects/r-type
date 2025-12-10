#include "protocol/sequence_tracker.h"

#include "protocol/reliability.h"

namespace protocol {

SequenceTracker::SequenceTracker(SequenceTracker&& other) noexcept {
  const std::lock_guard<std::mutex> lock(other.mutex_);
  local_sequence_ = other.local_sequence_;
  remote_sequence_ = other.remote_sequence_;
  remote_ack_bits_ = other.remote_ack_bits_;
  has_remote_ = other.has_remote_;
}

SequenceTracker& SequenceTracker::operator=(
    SequenceTracker&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  const std::scoped_lock lock(mutex_, other.mutex_);
  local_sequence_ = other.local_sequence_;
  remote_sequence_ = other.remote_sequence_;
  remote_ack_bits_ = other.remote_ack_bits_;
  has_remote_ = other.has_remote_;
  return *this;
}

std::uint32_t SequenceTracker::NextLocalSequence() {
  const std::lock_guard<std::mutex> lock(mutex_);
  ++local_sequence_;
  return local_sequence_;
}

void SequenceTracker::Reset() {
  const std::lock_guard<std::mutex> lock(mutex_);
  local_sequence_ = 0;
  remote_sequence_ = 0;
  remote_ack_bits_ = 0;
  has_remote_ = false;
}

void SequenceTracker::OnRemoteSequenceReceived(std::uint32_t sequence) {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (!has_remote_) {
    has_remote_ = true;
    remote_sequence_ = sequence;
    remote_ack_bits_ = 0;
    return;
  }

  if (sequence == remote_sequence_) {
    return;
  }

  if (IsSequenceMoreRecent(sequence, remote_sequence_)) {
    std::uint32_t delta = sequence - remote_sequence_;

    if (delta >= kAckBitsWindow) {
      remote_ack_bits_ = 0;
    } else {
      remote_ack_bits_ <<= delta;
      remote_ack_bits_ |= 1u << (delta - 1u);
    }

    remote_sequence_ = sequence;
  } else {
    std::uint32_t delta = remote_sequence_ - sequence;
    if (delta == 0 || delta > kAckBitsWindow) {
      return;
    }
    remote_ack_bits_ |= 1u << (delta - 1u);
  }
}

void SequenceTracker::FillAckFields(Header& header) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (!has_remote_) {
    return;
  }
  header.ack = remote_sequence_;
  header.ack_bits = remote_ack_bits_;
}

}  // namespace protocol
