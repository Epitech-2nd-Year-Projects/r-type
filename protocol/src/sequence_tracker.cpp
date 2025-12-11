#include "protocol/sequence_tracker.h"

#include "protocol/reliability.h"

namespace protocol {

std::uint32_t SequenceTracker::NextLocalSequence() {
  ++local_sequence_;
  return local_sequence_;
}

void SequenceTracker::Reset() {
  local_sequence_ = 0;
  remote_sequence_ = 0;
  remote_ack_bits_ = 0;
  has_remote_ = false;
}

void SequenceTracker::OnRemoteSequenceReceived(std::uint32_t sequence) {
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
  if (!has_remote_) {
    return;
  }
  header.ack = remote_sequence_;
  header.ack_bits = remote_ack_bits_;
}

}  // namespace protocol
