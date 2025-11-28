#ifndef PROTOCOL_SEQUENCE_TRACKER_H_
#define PROTOCOL_SEQUENCE_TRACKER_H_

#include <cstdint>
#include "protocol/header.h"

namespace protocol {
  class SequenceTracker {
    public:
      /**
       * @brief Number of bits in the ack_bits field.
       */
      static constexpr std::uint32_t kAckBitsWindow = 32u;

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
       * Handles duplicate packets, out-of-order packets, and newer packets.
       * This will update:
       *   - remote_sequence_ (highest sequence seen so far)
       *   - remote_ack_bits_ (bitmap for the last 32 sequences before remote_sequence_)
       *   - has_remote_ (set to true on first packet)
       */
      void OnRemoteSequenceReceived(std::uint32_t remote_sequence);

      /**
       * @brief Fills the ack and ack_bits fields of the given header with the current
       *        remote sequence tracking information.
       * @param header Pointer to the header structure to fill.
       * @note Does nothing if header is nullptr or if no remote packet has been received yet.
       */
      void FillAckFields(Header* header) const;

      /**
       * @brief Resets the sequence tracker to its initial state.
       */
      void Reset();

      /**
       * @brief Accessors (useful for debugging / tests).
       * @{
       */
      std::uint32_t local_sequence() const { return local_sequence_; }
      std::uint32_t remote_sequence() const { return remote_sequence_; }
      std::uint32_t remote_ack_bits() const { return remote_ack_bits_; }
      bool has_remote() const { return has_remote_; }
      /** @} */

    private:
      std::uint32_t local_sequence_ = 0;   ///< Next sequence number for outgoing packets.
      std::uint32_t remote_sequence_ = 0;  ///< Highest remote sequence number received.
      std::uint32_t remote_ack_bits_ = 0;  ///< Bitmap of received packets before remote_sequence_.
      bool has_remote_ = false;            ///< Whether any remote packet has been received.
  };
}

#endif // !PROTOCOL_SEQUENCE_TRACKER_H_
