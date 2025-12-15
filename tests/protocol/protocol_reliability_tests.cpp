#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>

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

TEST(ProtocolReliabilityTests, SequenceMoreRecentSimple) {
  using protocol::IsSequenceMoreRecent;

  EXPECT_TRUE(IsSequenceMoreRecent(5u, 4u));
  EXPECT_FALSE(IsSequenceMoreRecent(4u, 5u));
  EXPECT_FALSE(IsSequenceMoreRecent(10u, 10u));
}

TEST(ProtocolReliabilityTests, SequenceMoreRecentWrapAround) {
  using protocol::IsSequenceMoreRecent;
  using protocol::kSequenceHalfRange;

  const std::uint32_t max = 0xFFFFFFFFu;
  const std::uint32_t zero = 0u;

  EXPECT_TRUE(IsSequenceMoreRecent(zero, max));
  EXPECT_FALSE(IsSequenceMoreRecent(max, zero));
  EXPECT_FALSE(IsSequenceMoreRecent(max - 10u, max));
  EXPECT_TRUE(IsSequenceMoreRecent(max, max - 10u));

  (void)kSequenceHalfRange;
}

TEST(ProtocolReliabilityTests, IsPacketAckedBasics) {
  using protocol::IsPacketAcked;

  const std::uint32_t ack = 100u;
  const std::uint32_t ack_bits = 0b00000000000000000000000000000001u;

  EXPECT_TRUE(IsPacketAcked(100u, ack, ack_bits));
  EXPECT_TRUE(IsPacketAcked(99u, ack, ack_bits));
  EXPECT_FALSE(IsPacketAcked(98u, ack, ack_bits));

  EXPECT_FALSE(IsPacketAcked(130u, ack, ack_bits));

  const std::uint32_t very_old = ack - 40u;
  EXPECT_FALSE(IsPacketAcked(very_old, ack, ack_bits));
}

TEST(ProtocolReliabilityTests, SequenceTrackerBasic) {
  protocol::SequenceTracker tracker;
  protocol::Header header{};

  tracker.OnRemoteSequenceReceived(100u);
  tracker.FillAckFields(header);

  EXPECT_EQ(header.ack, 100u);
  EXPECT_EQ(header.ack_bits, 0u);

  tracker.OnRemoteSequenceReceived(99u);
  tracker.FillAckFields(header);

  EXPECT_EQ(header.ack, 100u);
  EXPECT_EQ(header.ack_bits, 1u);

  EXPECT_TRUE(protocol::IsPacketAcked(100u, header.ack, header.ack_bits));
  EXPECT_TRUE(protocol::IsPacketAcked(99u, header.ack, header.ack_bits));
  EXPECT_FALSE(protocol::IsPacketAcked(98u, header.ack, header.ack_bits));
}

TEST(ProtocolReliabilityTests, ReliabilityPolicyBasics) {
  using protocol::message_type::MessageType;

  EXPECT_TRUE(protocol::IsReliable(MessageType::kJoinRequest));
  EXPECT_TRUE(protocol::IsConnectionless(MessageType::kJoinRequest));

  EXPECT_FALSE(protocol::IsReliable(MessageType::kInputState));
  EXPECT_FALSE(protocol::IsConnectionless(MessageType::kInputState));

  EXPECT_FALSE(protocol::IsReliable(MessageType::kWorldSnapshot));

  EXPECT_TRUE(protocol::IsReliable(MessageType::kPlayerDied));

  EXPECT_FALSE(protocol::IsReliable(MessageType::kPing));
  EXPECT_TRUE(protocol::IsConnectionless(MessageType::kPing));
}

TEST(ProtocolReliabilityTests, ReliableQueueAckBasics) {
  protocol::ReliableQueue queue(/*resend_timeout_ms=*/100, /*max_pending=*/8);

  engine::net::PacketBuffer::Storage dummy_bytes = {0x01, 0x02, 0x03};

  queue.AddSentPacket(10u, dummy_bytes, /*now_ms=*/0u);
  queue.AddSentPacket(11u, dummy_bytes, /*now_ms=*/0u);
  queue.AddSentPacket(12u, dummy_bytes, /*now_ms=*/0u);

  EXPECT_EQ(queue.pending_count(), 3u);

  const std::uint32_t ack = 11u;
  const std::uint32_t ack_bits = 0x00000001u;

  queue.OnAckReceived(ack, ack_bits);

  EXPECT_EQ(queue.pending_count(), 1u);

  std::vector<protocol::PendingPacket> to_resend;
  queue.CollectPacketsToResend(/*now_ms=*/0u, to_resend);
  EXPECT_TRUE(to_resend.empty());
}

TEST(ProtocolReliabilityTests, ReliableQueueTimeoutResend) {
  protocol::ReliableQueue queue(/*resend_timeout_ms=*/100, /*max_pending=*/8);
  engine::net::PacketBuffer::Storage dummy_bytes = {0xAA, 0xBB};

  queue.AddSentPacket(20u, dummy_bytes, /*now_ms=*/0u);

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(/*now_ms=*/50u, to_resend);
    EXPECT_TRUE(to_resend.empty());
  }

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(150u, to_resend);
    ASSERT_EQ(to_resend.size(), 1u);
    EXPECT_EQ(to_resend[0].sequence, 20u);
  }

  {
    std::vector<protocol::PendingPacket> to_resend;
    queue.CollectPacketsToResend(/*now_ms=*/260u, to_resend);
    ASSERT_EQ(to_resend.size(), 1u);
    EXPECT_EQ(to_resend[0].sequence, 20u);
  }
}

TEST(ProtocolReliabilityTests, LatencyEstimatorNoOffset) {
  protocol::LatencyEstimator estimator;
  estimator.OnPingSent(100u);
  estimator.OnPongReceived(100u, 200u, 300u);

  ASSERT_TRUE(estimator.has_estimate());

  const float rtt = estimator.rtt_ms();
  const float offset = estimator.clock_offset_ms();

  EXPECT_GE(rtt, 199.5f);
  EXPECT_LE(rtt, 200.5f);

  EXPECT_GE(offset, -0.5f);
  EXPECT_LE(offset, 0.5f);
}

TEST(ProtocolReliabilityTests, LatencyEstimatorWithOffset) {
  protocol::LatencyEstimator estimator;
  estimator.OnPingSent(100u);
  estimator.OnPongReceived(100u, 250u, 300u);

  ASSERT_TRUE(estimator.has_estimate());

  const float rtt = estimator.rtt_ms();
  const float offset = estimator.clock_offset_ms();

  EXPECT_GE(rtt, 199.5f);
  EXPECT_LE(rtt, 200.5f);

  EXPECT_GE(offset, 49.5f);
  EXPECT_LE(offset, 50.5f);
}

TEST(ProtocolReliabilityTests, PacketDecodeInvalidVersion) {
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
  const bool ok = protocol::DecodePacket(buffer, decoded, error);

  EXPECT_FALSE(ok);
  EXPECT_EQ(error, protocol::DecodeError::kVersionMismatch);
}

TEST(ProtocolReliabilityTests, PacketDecodeInvalidMessageType) {
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
  const bool ok = protocol::DecodePacket(buffer, decoded, error);

  EXPECT_FALSE(ok);
  EXPECT_EQ(error, protocol::DecodeError::kUnknownMessageType);
}

TEST(ProtocolReliabilityTests, PacketDecodeUnexpectedEndOfBuffer) {
  engine::net::PacketBuffer buffer;
  buffer.WriteUint16(protocol::kProtocolVersion);
  buffer.WriteUint8(
      static_cast<std::uint8_t>(protocol::message_type::MessageType::kPing));

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, error);

  EXPECT_FALSE(ok);
  EXPECT_EQ(error, protocol::DecodeError::kUnexpectedEndOfBuffer);
}

TEST(ProtocolReliabilityTests, PacketDecodeInvalidPayload) {
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
  const bool ok = protocol::DecodePacket(buffer, decoded, error);

  EXPECT_FALSE(ok);
  EXPECT_TRUE(error == protocol::DecodeError::kInvalidPayload ||
              error == protocol::DecodeError::kUnexpectedEndOfBuffer);
}

TEST(ProtocolReliabilityTests, DecodeMetricsBasic) {
  protocol::DecodeMetrics metrics{};

  EXPECT_EQ(metrics.total_packets, 0);
  EXPECT_EQ(metrics.rejected_packets, 0);

  protocol::UpdateDecodeMetrics(metrics, protocol::DecodeError::kOk);
  EXPECT_EQ(metrics.total_packets, 1);
  EXPECT_EQ(metrics.rejected_packets, 0);

  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kVersionMismatch);
  EXPECT_EQ(metrics.total_packets, 2);
  EXPECT_EQ(metrics.rejected_packets, 1);
  EXPECT_EQ(metrics.version_mismatch, 1);

  protocol::UpdateDecodeMetrics(metrics,
                                protocol::DecodeError::kUnknownMessageType);
  EXPECT_EQ(metrics.total_packets, 3);
  EXPECT_EQ(metrics.rejected_packets, 2);
  EXPECT_EQ(metrics.unknown_message_type, 1);
}

TEST(ProtocolReliabilityTests, DecodeMetricsAllErrors) {
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

  EXPECT_EQ(metrics.total_packets, 6);
  EXPECT_EQ(metrics.rejected_packets, 6);
  EXPECT_EQ(metrics.unexpected_end_of_buffer, 1);
  EXPECT_EQ(metrics.invalid_header, 1);
  EXPECT_EQ(metrics.unknown_message_type, 1);
  EXPECT_EQ(metrics.version_mismatch, 1);
  EXPECT_EQ(metrics.invalid_payload, 1);
  EXPECT_EQ(metrics.invalid_snapshot_id, 1);
}

TEST(ProtocolReliabilityTests, DecodeErrorToString) {
  const auto str_ok = protocol::DecodeErrorToString(protocol::DecodeError::kOk);
  const auto str_eob = protocol::DecodeErrorToString(
      protocol::DecodeError::kUnexpectedEndOfBuffer);
  const auto str_hdr =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidHeader);
  const auto str_type =
      protocol::DecodeErrorToString(protocol::DecodeError::kUnknownMessageType);
  const auto str_ver =
      protocol::DecodeErrorToString(protocol::DecodeError::kVersionMismatch);
  const auto str_pay =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidPayload);
  const auto str_snap =
      protocol::DecodeErrorToString(protocol::DecodeError::kInvalidSnapshotId);

  EXPECT_FALSE(str_ok.empty());
  EXPECT_FALSE(str_eob.empty());
  EXPECT_FALSE(str_hdr.empty());
  EXPECT_FALSE(str_type.empty());
  EXPECT_FALSE(str_ver.empty());
  EXPECT_FALSE(str_pay.empty());
  EXPECT_FALSE(str_snap.empty());
}

TEST(ProtocolReliabilityTests, PacketDecodeEmptyBuffer) {
  engine::net::PacketBuffer buffer;

  protocol::Packet decoded{};
  protocol::DecodeError error = protocol::DecodeError::kOk;
  const bool ok = protocol::DecodePacket(buffer, decoded, error);

  EXPECT_FALSE(ok);
  EXPECT_EQ(error, protocol::DecodeError::kUnexpectedEndOfBuffer);
}

TEST(ProtocolReliabilityTests, PacketDecodeMultipleErrors) {
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

  EXPECT_EQ(metrics.total_packets, 18);
  EXPECT_EQ(metrics.rejected_packets, 15);
  EXPECT_EQ(metrics.version_mismatch, 10);
  EXPECT_EQ(metrics.unknown_message_type, 5);
}

TEST(ProtocolReliabilityTests, SnapshotHistoryBasic) {
  protocol::SnapshotHistory history(5);

  EXPECT_EQ(history.size(), 0u);
  EXPECT_EQ(history.capacity(), 5u);

  protocol::WorldSnapshotPayload snapshot1;
  snapshot1.snapshot_id = 100;
  snapshot1.server_tick = 1000;
  history.AddSnapshot(snapshot1);

  EXPECT_EQ(history.size(), 1u);

  const auto retrieved = history.GetSnapshot(100);
  ASSERT_TRUE(retrieved);
  EXPECT_EQ(retrieved->get().snapshot_id, 100u);
  EXPECT_EQ(retrieved->get().server_tick, 1000u);
}

TEST(ProtocolReliabilityTests, SnapshotHistoryCapacity) {
  protocol::SnapshotHistory history(3);

  for (std::uint32_t i = 0; i < 3; ++i) {
    protocol::WorldSnapshotPayload snapshot;
    snapshot.snapshot_id = 100 + i;
    snapshot.server_tick = 1000 + i;
    history.AddSnapshot(snapshot);
  }

  EXPECT_EQ(history.size(), 3u);

  EXPECT_TRUE(history.Contains(100));
  EXPECT_TRUE(history.Contains(101));
  EXPECT_TRUE(history.Contains(102));

  protocol::WorldSnapshotPayload snapshot4;
  snapshot4.snapshot_id = 103;
  snapshot4.server_tick = 1003;
  history.AddSnapshot(snapshot4);

  EXPECT_EQ(history.size(), 3u);

  EXPECT_FALSE(history.Contains(100));
  EXPECT_TRUE(history.Contains(101));
  EXPECT_TRUE(history.Contains(102));
  EXPECT_TRUE(history.Contains(103));
}

TEST(ProtocolReliabilityTests, SnapshotHistoryGetLatest) {
  protocol::SnapshotHistory history(5);

  EXPECT_FALSE(history.GetLatestSnapshot().has_value());

  for (std::uint32_t i = 0; i < 3; ++i) {
    protocol::WorldSnapshotPayload snapshot;
    snapshot.snapshot_id = 200 + i;
    snapshot.server_tick = 2000 + i;
    history.AddSnapshot(snapshot);
  }

  const auto latest = history.GetLatestSnapshot();
  ASSERT_TRUE(latest);
  EXPECT_EQ(latest->get().snapshot_id, 202u);
  EXPECT_EQ(latest->get().server_tick, 2002u);
}

TEST(ProtocolReliabilityTests, SnapshotHistoryWithDeltas) {
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

  const auto retrieved = history.GetSnapshot(300);
  ASSERT_TRUE(retrieved);
  ASSERT_EQ(retrieved->get().deltas.size(), 2u);
  EXPECT_EQ(retrieved->get().deltas[0].entity_id, 1u);
  EXPECT_EQ(retrieved->get().deltas[1].entity_id, 2u);
}

TEST(ProtocolReliabilityTests, SnapshotHistoryGetSnapshotNotFound) {
  protocol::SnapshotHistory history(5);

  protocol::WorldSnapshotPayload snapshot;
  snapshot.snapshot_id = 400;
  history.AddSnapshot(snapshot);

  const auto not_found = history.GetSnapshot(999);
  EXPECT_FALSE(not_found.has_value());

  EXPECT_FALSE(history.Contains(999));
}

}  // namespace
