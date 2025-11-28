#ifndef PROTOCOL_RELIABILITY_H_
#define PROTOCOL_RELIABILITY_H_

#include <cstdint>

namespace protocol {

  /**
   * @brief Half range for sequence number comparison (2^31).
   * 
   * Used to determine if a sequence number is more recent using wrap-around arithmetic.
   */
  inline constexpr std::uint32_t kSequenceHalfRange = 1u << 31;

  /**
   * @brief Determines if seq1 is more recent than seq2 using wrap-around arithmetic.
   * @param seq1 First sequence number to compare.
   * @param seq2 Second sequence number to compare.
   * @return true if seq1 is more recent than seq2, false otherwise.
   * 
   * Handles 32-bit unsigned integer wrap-around correctly.
   */
  bool IsSequenceMoreRecent(std::uint32_t seq1, std::uint32_t seq2);

  /**
   * @brief Determines if seq1 is older than seq2 using wrap-around arithmetic.
   * @param seq1 First sequence number to compare.
   * @param seq2 Second sequence number to compare.
   * @return true if seq1 is older than seq2, false otherwise.
   */
  inline bool IsSequenceOlder(std::uint32_t seq1, std::uint32_t seq2) {
    return !IsSequenceMoreRecent(seq1, seq2) && seq1 != seq2;
  }

  /**
   * @brief Checks if a packet with a given sequence number has been acknowledged.
   * @param packet_sequence The sequence number of the packet to check.
   * @param ack The most recent acknowledged sequence number.
   * @param ack_bits Bitmap of the 32 packets before ack.
   * @return true if the packet has been acknowledged, false otherwise.
   * 
   * Uses the ack and ack_bits fields from the protocol header to determine
   * if a specific packet sequence has been received by the remote peer.
   */
  bool IsPacketAcked(std::uint32_t packet_sequence,
                       std::uint32_t ack,
                       std::uint32_t ack_bits);
}

#endif // !PROTOCOL_RELIABILITY_H_
