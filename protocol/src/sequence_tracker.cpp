#include "protocol/sequence_tracker.h"
#include "protocol/header.h"

namespace protocol {

std::uint32_t SequenceTracker::NextLocalSequence() {
    ++local_sequence_;
    return local_sequence_;
}

void SequenceTracker::OnRemoteSequenceReceived(std::uint32_t remote_sequence) {
  if (remote_sequence == 0) {
    remote_sequence_ = remote_sequence;
    remote_ack_bits_ = 0;
    return;
  }

  if (remote_sequence > remote_sequence_) {
    std::uint32_t delta = remote_sequence - remote_sequence_;
    if (delta >= 32) {
      remote_ack_bits_ = 0;
    } else {
      remote_ack_bits_ <<= delta;
      remote_ack_bits_ |= (1u << (delta - 1));
    }
    remote_sequence_ = remote_sequence;
  } else if (remote_sequence < remote_sequence_) {
    std::uint32_t delta = remote_sequence_ - remote_sequence;
    if (delta <= 32) {
      remote_ack_bits_ |= (1u << (delta - 1));
    }
  }
}

void SequenceTracker::FillAckFields(struct Header* header) const {
  if (header != nullptr) {
    header->ack = remote_sequence_;
    header->ack_bits = remote_ack_bits_;
  }
}
}
