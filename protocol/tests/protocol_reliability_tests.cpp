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
#include "protocol/snapshot_history.h"

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

  if (!protocol::IsReliable(MessageType::kJoinRequest)) {
    std::cout << "JoinRequest should be reliable\n";
    return false;
  }
  if (!protocol::IsConnectionless(MessageType::kJoinRequest)) {
    std::cout << "JoinRequest should be connectionless\n";
    return false;
  }

  if (protocol::IsReliable(MessageType::kInputState)) {
    std::cout << "InputState should NOT be reliable\n";
    return false;
  }
  if (protocol::IsConnectionless(MessageType::kInputState)) {
    std::cout << "InputState should NOT be connectionless\n";
    return false;
  }

  if (protocol::IsReliable(MessageType::kWorldSnapshot)) {
    std::cout << "WorldSnapshot should NOT be reliable\n";
    return false;
  }

  if (!protocol::IsReliable(MessageType::kPlayerDied)) {
    std::cout << "PlayerDied should be reliable\n";
    return false;
  }

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

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for invalid version\n";
    return false;
  }
  if (error != protocol::DecodeError::kVersionMismatch) {
    std::cout << "Expected kVersionMismatch, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeInvalidMessageType() {
  protocol::Header header{};
  header.version = protocol::kProtocolVersion;
  header.message_type = 0xFF;
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
  if (error != protocol::DecodeError::kUnknownMessageType) {
    std::cout << "Expected kUnknownMessageType, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeUnexpectedEndOfBuffer() {
  engine::net::PacketBuffer buffer;
  buffer.WriteUint16(protocol::kProtocolVersion);
  buffer.WriteUint8(
      static_cast<std::uint8_t>(protocol::message_type::MessageType::kPing));

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for truncated header\n";
    return false;
  }
  if (error != protocol::DecodeError::kUnexpectedEndOfBuffer) {
    std::cout << "Expected kUnexpectedEndOfBuffer, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeInvalidPayload() {
  protocol::Header header{};
  header.version = protocol::kProtocolVersion;
  header.message_type = static_cast<std::uint8_t>(
      protocol::message_type::MessageType::kJoinRequest);
  header.flags = 0;
  header.sequence = 1;
  header.ack = 0;
  header.ack_bits = 0;
  header.timestamp_ms = 100;

  engine::net::PacketBuffer buffer;
  protocol::EncodeHeader(header, buffer);
  buffer.WriteUint8(0xFF);

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for invalid payload\n";
    return false;
  }
  if (error != protocol::DecodeError::kInvalidPayload &&
      error != protocol::DecodeError::kUnexpectedEndOfBuffer) {
    std::cout << "Expected kInvalidPayload or kUnexpectedEndOfBuffer, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestDecodeMetricsBasic() {
  protocol::DecodeMetrics metrics{};

  if (metrics.total_packets != 0 || metrics.rejected_packets != 0) {
    std::cout << "Expected initial metrics to be zero\n";
    return false;
  }

  protocol::UpdateDecodeMetrics(metrics, protocol::DecodeError::kOk);
  if (metrics.total_packets != 1 || metrics.rejected_packets != 0) {
    std::cout << "Expected total=1, rejected=0 after success\n";
    return false;
  }

  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kVersionMismatch);
  if (metrics.total_packets != 2 || metrics.rejected_packets != 1 ||
      metrics.version_mismatch != 1) {
    std::cout << "Expected total=2, rejected=1, version_mismatch=1\n";
    return false;
  }

  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kUnknownMessageType);
  if (metrics.total_packets != 3 || metrics.rejected_packets != 2 ||
      metrics.unknown_message_type != 1) {
    std::cout << "Expected total=3, rejected=2, unknown_message_type=1\n";
    return false;
  }

  return true;
}

bool TestDecodeMetricsAllErrors() {
  protocol::DecodeMetrics metrics{};

  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kUnexpectedEndOfBuffer);
  protocol::UpdateDecodeMetrics(metrics, protocol::DecodeError::kInvalidHeader);
  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kUnknownMessageType);
  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kVersionMismatch);
  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kInvalidPayload);
  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kInvalidSnapshotId);

  if (metrics.total_packets != 6) {
    std::cout << "Expected 6 total packets, got " << metrics.total_packets
              << "\n";
    return false;
  }
  if (metrics.rejected_packets != 6) {
    std::cout << "Expected 6 rejected packets, got " << metrics.rejected_packets
              << "\n";
    return false;
  }
  if (metrics.unexpected_end_of_buffer != 1 || metrics.invalid_header != 1 ||
      metrics.unknown_message_type != 1 || metrics.version_mismatch != 1 ||
      metrics.invalid_payload != 1 || metrics.invalid_snapshot_id != 1) {
    std::cout << "Error counters mismatch\n";
    return false;
  }

  return true;
}

bool TestDecodeErrorToString() {
  const char* str_ok =
      protocol::DecodeErrorToString(protocol::DecodeError::kOk);
  const char* str_eob = protocol::DecodeErrorToString(
      protocol::DecodeError::kUnexpectedEndOfBuffer);
  const char* str_hdr =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidHeader);
  const char* str_type =
      protocol::DecodeErrorToString(protocol::DecodeError::kUnknownMessageType);
  const char* str_ver =
      protocol::DecodeErrorToString(protocol::DecodeError::kVersionMismatch);
  const char* str_pay =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidPayload);
  const char* str_snap =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidSnapshotId);

  if (!str_ok || !str_eob || !str_hdr || !str_type || !str_ver || !str_pay ||
      !str_snap) {
    std::cout << "One or more error strings are null\n";
    return false;
  }

  if (str_ok[0] == '\0' || str_eob[0] == '\0' || str_hdr[0] == '\0' ||
      str_type[0] == '\0' || str_ver[0] == '\0' || str_pay[0] == '\0' ||
      str_snap[0] == '\0') {
    std::cout << "One or more error strings are empty\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeEmptyBuffer() {
  engine::net::PacketBuffer buffer;

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, &error);

  if (ok) {
    std::cout << "Expected DecodePacket to fail for empty buffer\n";
    return false;
  }
  if (error != protocol::DecodeError::kUnexpectedEndOfBuffer) {
    std::cout << "Expected kUnexpectedEndOfBuffer for empty buffer, got "
              << protocol::DecodeErrorToString(error) << "\n";
    return false;
  }

  return true;
}

bool TestPacketDecodeMultipleErrors() {
  protocol::DecodeMetrics metrics{};

  for (int i = 0; i < 10; ++i) {
    protocol::UpdateDecodeMetrics(metrics,
                                  protocol::DecodeError::kVersionMismatch);
  }
  for (int i = 0; i < 5; ++i) {
    protocol::UpdateDecodeMetrics(metrics,
                                  protocol::DecodeError::kUnknownMessageType);
  }
  for (int i = 0; i < 3; ++i) {
    protocol::UpdateDecodeMetrics(metrics, protocol::DecodeError::kOk);
  }

  if (metrics.total_packets != 18) {
    std::cout << "Expected 18 total packets, got " << metrics.total_packets
              << "\n";
    return false;
  }
  if (metrics.rejected_packets != 15) {
    std::cout << "Expected 15 rejected packets, got "
              << metrics.rejected_packets << "\n";
    return false;
  }
  if (metrics.version_mismatch != 10) {
    std::cout << "Expected 10 version_mismatch, got "
              << metrics.version_mismatch << "\n";
    return false;
  }
  if (metrics.unknown_message_type != 5) {
    std::cout << "Expected 5 unknown_message_type, got "
              << metrics.unknown_message_type << "\n";
    return false;
  }

  return true;
}

bool TestSnapshotHistoryBasic() {
  protocol::SnapshotHistory history(5);

  if (history.size() != 0) {
    std::cout << "Expected empty history, got size " << history.size() << "\n";
    return false;
  }
  if (history.capacity() != 5) {
    std::cout << "Expected capacity 5, got " << history.capacity() << "\n";
    return false;
  }

  protocol::WorldSnapshotPayload snapshot1;
  snapshot1.snapshot_id = 100;
  snapshot1.server_tick = 1000;
  history.AddSnapshot(snapshot1);

  if (history.size() != 1) {
    std::cout << "Expected size 1 after adding snapshot, got " << history.size()
              << "\n";
    return false;
  }

  const protocol::WorldSnapshotPayload* retrieved = history.GetSnapshot(100);
  if (!retrieved) {
    std::cout << "Failed to retrieve snapshot with ID 100\n";
    return false;
  }
  if (retrieved->snapshot_id != 100 || retrieved->server_tick != 1000) {
    std::cout << "Retrieved snapshot has wrong data\n";
    return false;
  }

  return true;
}

bool TestSnapshotHistoryCapacity() {
  protocol::SnapshotHistory history(3);

  for (std::uint32_t i = 0; i < 3; ++i) {
    protocol::WorldSnapshotPayload snapshot;
    snapshot.snapshot_id = 100 + i;
    snapshot.server_tick = 1000 + i;
    history.AddSnapshot(snapshot);
  }

  if (history.size() != 3) {
    std::cout << "Expected size 3, got " << history.size() << "\n";
    return false;
  }

  if (!history.Contains(100) || !history.Contains(101) ||
      !history.Contains(102)) {
    std::cout << "Missing snapshots 100-102\n";
    return false;
  }

  protocol::WorldSnapshotPayload snapshot4;
  snapshot4.snapshot_id = 103;
  snapshot4.server_tick = 1003;
  history.AddSnapshot(snapshot4);

  if (history.size() != 3) {
    std::cout << "Expected size 3 after overflow, got " << history.size()
              << "\n";
    return false;
  }

  if (history.Contains(100)) {
    std::cout << "Snapshot 100 should have been evicted\n";
    return false;
  }

  if (!history.Contains(101) || !history.Contains(102) ||
      !history.Contains(103)) {
    std::cout << "Missing snapshots 101-103\n";
    return false;
  }

  return true;
}

bool TestSnapshotHistoryGetLatest() {
  protocol::SnapshotHistory history(5);

  if (history.GetLatestSnapshot() != nullptr) {
    std::cout << "Expected nullptr for empty history\n";
    return false;
  }

  for (std::uint32_t i = 0; i < 3; ++i) {
    protocol::WorldSnapshotPayload snapshot;
    snapshot.snapshot_id = 200 + i;
    snapshot.server_tick = 2000 + i;
    history.AddSnapshot(snapshot);
  }

  const protocol::WorldSnapshotPayload* latest = history.GetLatestSnapshot();
  if (!latest) {
    std::cout << "Failed to get latest snapshot\n";
    return false;
  }
  if (latest->snapshot_id != 202 || latest->server_tick != 2002) {
    std::cout << "Latest snapshot has wrong data: id=" << latest->snapshot_id
              << " tick=" << latest->server_tick << "\n";
    return false;
  }

  return true;
}

bool TestSnapshotHistoryWithDeltas() {
  protocol::SnapshotHistory history(10);

  protocol::WorldSnapshotPayload snapshot;
  snapshot.snapshot_id = 300;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.server_tick = 3000;

  protocol::EntityDelta delta1;
  delta1.op = protocol::EntityDeltaOp::kCreate;
  delta1.entity_id = 1;
  delta1.state.entity_id = 1;
  delta1.state.type = 10;
  delta1.state.x = 100;
  delta1.state.y = 200;

  protocol::EntityDelta delta2;
  delta2.op = protocol::EntityDeltaOp::kCreate;
  delta2.entity_id = 2;
  delta2.state.entity_id = 2;
  delta2.state.type = 20;
  delta2.state.x = 150;
  delta2.state.y = 250;

  snapshot.deltas.push_back(delta1);
  snapshot.deltas.push_back(delta2);

  history.AddSnapshot(snapshot);

  const protocol::WorldSnapshotPayload* retrieved = history.GetSnapshot(300);
  if (!retrieved) {
    std::cout << "Failed to retrieve snapshot 300\n";
    return false;
  }
  if (retrieved->deltas.size() != 2) {
    std::cout << "Expected 2 deltas, got " << retrieved->deltas.size() << "\n";
    return false;
  }
  if (retrieved->deltas[0].entity_id != 1 ||
      retrieved->deltas[1].entity_id != 2) {
    std::cout << "Delta entity IDs don't match\n";
    return false;
  }

  return true;
}

bool TestSnapshotHistoryGetSnapshotNotFound() {
  protocol::SnapshotHistory history(5);

  protocol::WorldSnapshotPayload snapshot;
  snapshot.snapshot_id = 400;
  history.AddSnapshot(snapshot);

  const protocol::WorldSnapshotPayload* not_found = history.GetSnapshot(999);
  if (not_found != nullptr) {
    std::cout << "Expected nullptr for non-existent snapshot\n";
    return false;
  }

  if (history.Contains(999)) {
    std::cout << "Contains should return false for non-existent snapshot\n";
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
  if (!RunTest("Packet Decode unexpected end of buffer",
               &TestPacketDecodeUnexpectedEndOfBuffer)) {
    ++failures;
  }
  if (!RunTest("Packet Decode invalid payload",
               &TestPacketDecodeInvalidPayload)) {
    ++failures;
  }
  if (!RunTest("Packet Decode empty buffer", &TestPacketDecodeEmptyBuffer)) {
    ++failures;
  }
  if (!RunTest("DecodeMetrics basic", &TestDecodeMetricsBasic)) {
    ++failures;
  }
  if (!RunTest("DecodeMetrics all errors", &TestDecodeMetricsAllErrors)) {
    ++failures;
  }
  if (!RunTest("DecodeMetrics multiple errors",
               &TestPacketDecodeMultipleErrors)) {
    ++failures;
  }
  if (!RunTest("DecodeErrorToString all values", &TestDecodeErrorToString)) {
    ++failures;
  }
  if (!RunTest("SnapshotHistory basic", &TestSnapshotHistoryBasic)) {
    ++failures;
  }
  if (!RunTest("SnapshotHistory capacity and eviction",
               &TestSnapshotHistoryCapacity)) {
    ++failures;
  }
  if (!RunTest("SnapshotHistory get latest", &TestSnapshotHistoryGetLatest)) {
    ++failures;
  }
  if (!RunTest("SnapshotHistory with deltas", &TestSnapshotHistoryWithDeltas)) {
    ++failures;
  }
  if (!RunTest("SnapshotHistory get snapshot not found",
               &TestSnapshotHistoryGetSnapshotNotFound)) {
    ++failures;
  }

  if (failures == 0) {
    std::cout << "All reliability tests passed.\n";
  } else {
    std::cout << failures << " reliability test(s) failed.\n";
  }

  return failures == 0 ? 0 : 1;
}
