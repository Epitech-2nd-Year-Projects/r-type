#include <cstdint>
#include <iostream>

#include "protocol/error.h"
#include "protocol/latency_estimator.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/reliability.h"
#include "protocol/reliability_policy.h"
#include "protocol/reliable_queue.h"
#include "protocol/sequence_tracker.h"

namespace {

template <typename F>
bool RunTest(const char* name, F&& fn) {
  const bool ok = fn();
  std::cout << (ok ? "[ OK ] " : "[FAIL] ") << name << "\n";
  return ok;
}

bool TestSequenceMoreRecentSimple() {
  using protocol::IsSequenceMoreRecent;

  if (!IsSequenceMoreRecent(5u, 4u)) return false;
  if (IsSequenceMoreRecent(4u, 5u)) return false;
  if (IsSequenceMoreRecent(10u, 10u)) return false;

  return true;
}

bool TestSequenceMoreRecentWrapAround() {
  using protocol::IsSequenceMoreRecent;
  using protocol::kSequenceHalfRange;

  const std::uint32_t max = 0xFFFFFFFFu;
  const std::uint32_t zero = 0u;

  if (!IsSequenceMoreRecent(zero, max)) return false;
  if (IsSequenceMoreRecent(max, zero)) return false;
  if (IsSequenceMoreRecent(max - 10u, max)) return false;
  if (!IsSequenceMoreRecent(max, max - 10u)) return false;

  (void)kSequenceHalfRange;

  return true;
}

bool TestIsPacketAckedBasics() {
  using protocol::IsPacketAcked;

  const std::uint32_t ack = 100u;
  const std::uint32_t ack_bits = 0b00000000000000000000000000000001u;

  if (!IsPacketAcked(100u, ack, ack_bits)) return false;
  if (!IsPacketAcked(99u, ack, ack_bits)) return false;
  if (IsPacketAcked(98u, ack, ack_bits)) return false;

  if (IsPacketAcked(130u, ack, ack_bits)) return false;

  const std::uint32_t very_old = ack - 40u;
  if (IsPacketAcked(very_old, ack, ack_bits)) return false;

  return true;
}

bool TestSequenceTrackerBasic() {
  protocol::SequenceTracker tracker;
  protocol::Header header{};

  tracker.OnRemoteSequenceReceived(100u);
  tracker.FillAckFields(&header);

  if (header.ack != 100u) {
    std::cout << "Expected ack=100 after first packet, got " << header.ack
              << "\n";
    return false;
  }
  if (header.ack_bits != 0u) {
    std::cout << "Expected ack_bits=0 after first packet, got "
              << header.ack_bits << "\n";
    return false;
  }

  tracker.OnRemoteSequenceReceived(99u);
  tracker.FillAckFields(&header);

  if (header.ack != 100u) {
    std::cout << "Expected ack=100 after second packet, got " << header.ack
              << "\n";
    return false;
  }

  if (header.ack_bits != 1u) {
    std::cout << "Expected ack_bits=1 (bit0 set for seq=99), got "
              << header.ack_bits << "\n";
    return false;
  }

  if (!protocol::IsPacketAcked(100u, header.ack, header.ack_bits)) {
    std::cout << "Packet 100 should be acked\n";
    return false;
  }
  if (!protocol::IsPacketAcked(99u, header.ack, header.ack_bits)) {
    std::cout << "Packet 99 should be acked through ack_bits\n";
    return false;
  }
  if (protocol::IsPacketAcked(98u, header.ack, header.ack_bits)) {
    std::cout << "Packet 98 should NOT be acked (bit1=0)\n";
    return false;
  }

  return true;
}

bool TestReliabilityPolicyBasics() {
  using protocol::message_type::MessageType;

  // JoinRequest : fiable + connectionless.
  if (!protocol::IsReliable(MessageType::kJoinRequest)) {
    std::cout << "JoinRequest should be reliable\n";
    return false;
  }
  if (!protocol::IsConnectionless(MessageType::kJoinRequest)) {
    std::cout << "JoinRequest should be connectionless\n";
    return false;
  }

  // InputState : pas reliable, pas connectionless.
  if (protocol::IsReliable(MessageType::kInputState)) {
    std::cout << "InputState should NOT be reliable\n";
    return false;
  }
  if (protocol::IsConnectionless(MessageType::kInputState)) {
    std::cout << "InputState should NOT be connectionless\n";
    return false;
  }

  // WorldSnapshot : pas reliable.
  if (protocol::IsReliable(MessageType::kWorldSnapshot)) {
    std::cout << "WorldSnapshot should NOT be reliable\n";
    return false;
  }

  // PlayerDied : reliable.
  if (!protocol::IsReliable(MessageType::kPlayerDied)) {
    std::cout << "PlayerDied should be reliable\n";
    return false;
  }

  // Ping : connectionless, pas reliable.
  if (protocol::IsReliable(MessageType::kPing)) {
    std::cout << "Ping should NOT be reliable\n";
    return false;
  }
  if (!protocol::IsConnectionless(MessageType::kPing)) {
    std::cout << "Ping should be connectionless\n";
    return false;
  }

  return true;
}

bool TestReliableQueueAckBasics() {
  protocol::ReliableQueue queue(/*resend_timeout_ms=*/100, /*max_pending=*/8);

  engine::net::PacketBuffer::Storage dummy_bytes = {0x01, 0x02, 0x03};

  queue.AddSentPacket(10u, dummy_bytes, /*now_ms=*/0u);
  queue.AddSentPacket(11u, dummy_bytes, /*now_ms=*/0u);
  queue.AddSentPacket(12u, dummy_bytes, /*now_ms=*/0u);

  if (queue.pending_count() != 3u) {
    std::cout << "Expected 3 pending packets, got " << queue.pending_count()
              << "\n";
    return false;
  }

  const std::uint32_t ack = 11u;
  const std::uint32_t ack_bits = 0x00000001u;

  queue.OnAckReceived(ack, ack_bits);

  if (queue.pending_count() != 1u) {
    std::cout << "Expected 1 pending packet after ACK, got "
              << queue.pending_count() << "\n";
    return false;
  }

  std::vector<protocol::PendingPacket> to_resend;
  queue.CollectPacketsToResend(/*now_ms=*/0u, &to_resend);
  if (!to_resend.empty()) {
    std::cout << "Expected no packets to resend yet\n";
    return false;
  }

  return true;
}

bool TestReliableQueueTimeoutResend() {
  protocol::ReliableQueue queue(/*resend_timeout_ms=*/100, /*max_pending=*/8);
  engine::net::PacketBuffer::Storage dummy_bytes = {0xAA, 0xBB};

  queue.AddSentPacket(20u, dummy_bytes, /*now_ms=*/0u);

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(/*now_ms=*/50u, &to_resend);
    if (!to_resend.empty()) {
      std::cout << "Expected no resend at 50 ms\n";
      return false;
    }
  }

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(150u, &to_resend);
    if (to_resend.size() != 1u) {
      std::cout << "Expected 1 packet to resend at 150 ms, got "
                << to_resend.size() << "\n";
      return false;
    }
    if (to_resend[0].sequence != 20u) {
      std::cout << "Expected sequence 20 to resend, got "
                << to_resend[0].sequence << "\n";
      return false;
    }
  }

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(/*now_ms=*/260u, &to_resend);
    if (to_resend.size() != 1u) {
      std::cout << "Expected 1 packet to resend at 260 ms, got "
                << to_resend.size() << "\n";
      return false;
    }
    if (to_resend[0].sequence != 20u) {
      std::cout << "Expected sequence 20 to resend again, got "
                << to_resend[0].sequence << "\n";
      return false;
    }
  }

  return true;
}

bool TestLatencyEstimatorNoOffset() {
  protocol::LatencyEstimator estimator;
  estimator.OnPingSent(100u);
  estimator.OnPongReceived(100u, 200u, 300u);

  if (!estimator.has_estimate()) {
    std::cout << "LatencyEstimator should have an estimate\n";
    return false;
  }

  const float rtt = estimator.rtt_ms();
  const float offset = estimator.clock_offset_ms();

  if (rtt < 199.5f || rtt > 200.5f) {
    std::cout << "Unexpected RTT: " << rtt << " (expected ~200)\n";
    return false;
  }

  if (offset < -0.5f || offset > 0.5f) {
    std::cout << "Unexpected offset: " << offset << " (expected ~0)\n";
    return false;
  }

  return true;
}

bool TestLatencyEstimatorWithOffset() {
  protocol::LatencyEstimator estimator;
  estimator.OnPingSent(100u);
  estimator.OnPongReceived(100u, 250u, 300u);

  if (!estimator.has_estimate()) {
    std::cout << "LatencyEstimator should have an estimate\n";
    return false;
  }

  const float rtt = estimator.rtt_ms();
  const float offset = estimator.clock_offset_ms();

  if (rtt < 199.5f || rtt > 200.5f) {
    std::cout << "Unexpected RTT with offset: " << rtt << " (expected ~200)\n";
    return false;
  }

  if (offset < 49.5f || offset > 50.5f) {
    std::cout << "Unexpected clock offset: " << offset << " (expected ~50)\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeInvalidVersion() {
  protocol::Header header{};
  header.version = protocol::kProtocolVersion + 1;
  header.message_type =
      static_cast<std::uint8_t>(protocol::message_type::MessageType::kPing);
  header.flags = 0;
  header.sequence = 1;
  header.ack = 0;
  header.ack_bits = 0;
  header.timestamp_ms = 1234;

  engine::net::PacketBuffer buffer;
  protocol::EncodeHeader(header, buffer);
  // Pas de payload derrière.

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for invalid version\n";
    return false;
  }
  if (error != protocol::DecodeError::kInvalidVersion) {
    std::cout << "Expected kInvalidVersion, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeInvalidMessageType() {
  protocol::Header header{};
  header.version = protocol::kProtocolVersion;
  header.message_type = 0xFF;  // valeur non mappée
  header.flags = 0;
  header.sequence = 1;
  header.ack = 0;
  header.ack_bits = 0;
  header.timestamp_ms = 42;

  engine::net::PacketBuffer buffer;
  protocol::EncodeHeader(header, buffer);

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for invalid message_type\n";
    return false;
  }
  if (error != protocol::DecodeError::kInvalidMessageType) {
    std::cout << "Expected kInvalidMessageType, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

}  // namespace

int RunProtocolReliabilityTests() {
  int failures = 0;

  if (!RunTest("SequenceMoreRecent simple", &TestSequenceMoreRecentSimple)) {
    ++failures;
  }
  if (!RunTest("SequenceMoreRecent wrap-around",
               &TestSequenceMoreRecentWrapAround)) {
    ++failures;
  }
  if (!RunTest("IsPacketAcked basics", &TestIsPacketAckedBasics)) {
    ++failures;
  }
  if (!RunTest("SequenceTracker basic", &TestSequenceTrackerBasic)) {
    ++failures;
  }
  if (!RunTest("ReliabilityPolicy basics", &TestReliabilityPolicyBasics)) {
    ++failures;
  }
  if (!RunTest("ReliableQueue ACK basics", &TestReliableQueueAckBasics)) {
    ++failures;
  }
  if (!RunTest("ReliableQueue timeout resend",
               &TestReliableQueueTimeoutResend)) {
    ++failures;
  }
  if (!RunTest("LatencyEstimator no offset", &TestLatencyEstimatorNoOffset)) {
    ++failures;
  }
  if (!RunTest("LatencyEstimator with offset",
               &TestLatencyEstimatorWithOffset)) {
    ++failures;
  }
  if (!RunTest("Packet Decode invalid version",
               &TestPacketDecodeInvalidVersion)) {
    ++failures;
  }
  if (!RunTest("Packet Decode invalid message type",
               &TestPacketDecodeInvalidMessageType)) {
    ++failures;
  }

  if (failures == 0) {
    std::cout << "All reliability tests passed.\n";
  } else {
    std::cout << failures << " reliability test(s) failed.\n";
  }

  return failures == 0 ? 0 : 1;
}
