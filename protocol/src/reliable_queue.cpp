#include "protocol/reliable_queue.h"

#include <algorithm>

namespace protocol {

ReliableQueue::ReliableQueue(std::uint32_t resend_timeout_ms,
                             std::size_t max_pending)
    : resend_timeout_ms_(resend_timeout_ms), max_pending_(max_pending) {}

void ReliableQueue::AddSentPacket(
    std::uint32_t sequence, const engine::net::PacketBuffer::Storage& bytes,
    std::uint32_t now_ms) {
  PendingPacket packet;
  packet.sequence = sequence;
  packet.bytes = bytes;
  packet.last_send_time_ms = now_ms;
  packet.send_count = 1;
  if (pending_.size() >= max_pending_ && !pending_.empty()) {
    pending_.erase(pending_.begin());
  }
  pending_.push_back(std::move(packet));
}

void ReliableQueue::OnAckReceived(std::uint32_t ack, std::uint32_t ack_bits) {
  if (pending_.empty()) {
    return;
  }
  auto keep_it =
      std::remove_if(pending_.begin(), pending_.end(),
                     [ack, ack_bits](const PendingPacket& p) {
                       return IsPacketAcked(p.sequence, ack, ack_bits);
                     });
  pending_.erase(keep_it, pending_.end());
}

void ReliableQueue::CollectPacketsToResend(
    std::uint32_t now_ms, std::vector<PendingPacket>* out_packets) {
  if (out_packets == nullptr) {
    return;
  }
  for (PendingPacket& packet : pending_) {
    const std::uint32_t elapsed_ms = now_ms - packet.last_send_time_ms;
    if (elapsed_ms >= resend_timeout_ms_) {
      out_packets->push_back(packet);
      packet.last_send_time_ms = now_ms;
      ++packet.send_count;
    }
  }
}
}  // namespace protocol
