#include "protocol/reliability.h"

namespace protocol {

bool IsSequenceMoreRecent(std::uint32_t seq1, std::uint32_t seq2) {
  return ((seq1 > seq2) && (seq1 - seq2 <= kSequenceHalfRange)) ||
         ((seq2 > seq1) && (seq2 - seq1 > kSequenceHalfRange));
}

bool IsPacketAcked(std::uint32_t packet_sequence, std::uint32_t ack,
                   std::uint32_t ack_bits) {
  if (packet_sequence == ack) {
    return true;
  }
  if (IsSequenceMoreRecent(packet_sequence, ack)) {
    return false;
  }
  std::uint32_t delta = ack - packet_sequence;
  if (delta == 0 || delta > 32u) {
    return false;
  }
  const std::uint32_t mask = 1u << (delta - 1u);
  return (ack_bits & mask) != 0;
}
}  // namespace protocol