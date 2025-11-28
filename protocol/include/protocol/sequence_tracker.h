#ifndef PROTOCOL_SEQUENCE_TRACKER_H_
#define PROTOCOL_SEQUENCE_TRACKER_H_

#include <cstdint>
#include "protocol/header.h"

namespace protocol {
  class SequenceTracker {
    public:
      SequenceTracker() = default;

      /**
       * @brief Returns the next local sequence number to use for an outgoing packet.
       * @return The next sequence number. The first call returns 1, then 2, 3, ...
       */
      std::uint32_t NextLocalSequence();
      
      /**
       * @brief Must be called whenever a packet from the remote peer is received.
       * @param remote_sequence The sequence number found in the remote header.
       * 
       * This will update:
       *   - remote_sequence_ (highest sequence seen so far)
       *   - remote_ack_bits_ (bitmap for the last 32 sequences before remote_sequence_)
       */
      void OnRemoteSequenceReceived(std::uint32_t remote_sequence);

      /**
       * @brief Fills the ack and ack_bits fields of the given header with the current
       *        remote sequence tracking information.
       * @param header Pointer to the header structure to fill. Does nothing if nullptr.
       */
      void FillAckFields(struct Header* header) const;

      /**
       * @brief Accessors (useful for debugging / tests).
       * @{
       */
      std::uint32_t local_sequence() const { return local_sequence_; }
      std::uint32_t remote_sequence() const { return remote_sequence_; }
      std::uint32_t remote_ack_bits() const { return remote_ack_bits_; }
      /** @} */

    private:
      std::uint32_t local_sequence_ = 0;
      std::uint32_t remote_sequence_ = 0;
      std::uint32_t remote_ack_bits_ = 0;
  };
}

#endif /* !PROTOCOL_SEQUENCE_TRACKER_H_ */
