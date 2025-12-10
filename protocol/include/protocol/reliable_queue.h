#ifndef PROTOCOL_RELIABLE_QUEUE_H_
#define PROTOCOL_RELIABLE_QUEUE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/net/packet_buffer.h"
#include "protocol/reliability.h"

namespace protocol {

/**
 * @brief One pending reliable packet tracked by ReliableQueue.
 */
struct PendingPacket {
  std::uint32_t sequence = 0;                     ///< Packet sequence number.
  engine::net::PacketBuffer::Storage bytes{};    ///< Encoded packet bytes (header + payload).
  std::uint32_t last_send_time_ms = 0;           ///< Last time this packet was sent (in milliseconds).
  std::uint32_t send_count = 0;                  ///< Number of times this packet has been sent.
};

/**
 * @brief Helper class to track reliable packets and schedule retransmissions.
 * 
 * Usage pattern (client or server side):
 *  - When sending a packet marked as "reliable":
 *      1) Assign it a sequence number via SequenceTracker
 *      2) Encode it into a PacketBuffer
 *      3) Call AddSentPacket(sequence, buffer.storage(), now_ms)
 * 
 *  - On receiving a remote header (ack/ack_bits):
 *      queue.OnAckReceived(header.ack, header.ack_bits);
 * 
 *  - Periodically (e.g., every tick):
 *      std::vector<PendingPacket> to_resend;
 *      queue.CollectPacketsToResend(now_ms, to_resend);
 *      // Re-send to_resend[i].bytes over the socket
 */
class ReliableQueue {
 public:
  /**
   * @brief Constructs a ReliableQueue with specified timeout and capacity.
   * @param resend_timeout_ms Timeout in milliseconds before retransmitting a packet.
   * @param max_pending Maximum number of pending packets allowed in the queue.
   */
  ReliableQueue(std::uint32_t resend_timeout_ms, std::size_t max_pending);

  /**
   * @brief Registers a newly sent reliable packet in the queue.
   * @param sequence Sequence number used in the packet header.
   * @param bytes Encoded packet bytes (header + payload).
   * @param now_ms Current time in milliseconds.
   * 
   * @note If the queue is full, the oldest pending packet is dropped to make room.
   */
  void AddSentPacket(std::uint32_t sequence,
                     const engine::net::PacketBuffer::Storage& bytes,
                     std::uint32_t now_ms);

  /**
   * @brief Informs the queue of remote ACK information.
   * @param ack The most recent acknowledged sequence number.
   * @param ack_bits Bitmap of the 32 packets before ack.
   * 
   * Any pending packet whose sequence is considered acknowledged by
   * IsPacketAcked(sequence, ack, ack_bits) will be removed from the queue.
   */
  void OnAckReceived(std::uint32_t ack, std::uint32_t ack_bits);

  /**
   * @brief Returns all packets that should be retransmitted.
   * @param now_ms Current time in milliseconds.
   * @param out_packets Output vector to receive packets needing retransmission.
   * 
   * A packet is considered timed-out if:
   *   (now_ms - last_send_time_ms) >= resend_timeout_ms
   * 
   * For each timed-out packet, this method:
   *   - Pushes a copy into out_packets
   *   - Updates last_send_time_ms to now_ms
   *   - Increments send_count
   */
  void CollectPacketsToResend(std::uint32_t now_ms,
                              std::vector<PendingPacket>& out_packets);

  /**
   * @brief Marks a resend attempt as failed to avoid delaying the next retry.
   * @param sequence Sequence number of the packet that failed to send.
   * @param now_ms Current time in milliseconds.
   *
   * Resets the packet's last_send_time_ms so it will be eligible for
   * retransmission on the next CollectPacketsToResend call.
   */
  void MarkSendFailed(std::uint32_t sequence, std::uint32_t now_ms);

  /**
   * @brief Number of packets still waiting for acknowledgment.
   * @return The count of pending packets in the queue.
   */
  std::size_t pending_count() const { return pending_.size(); }

 private:
  std::uint32_t resend_timeout_ms_;  ///< Timeout in milliseconds before retransmitting.
  std::size_t max_pending_;          ///< Maximum number of pending packets.
  // NOTE: pending_ uses std::vector; max_pending_ is expected to be small
  // (e.g. <= 64). If this ever grows large, consider switching to std::deque
  // to make front-erase O(1).
  std::vector<PendingPacket> pending_; ///< Queue of packets awaiting acknowledgment.
};

}  // namespace protocol

#endif  // PROTOCOL_RELIABLE_QUEUE_H_
