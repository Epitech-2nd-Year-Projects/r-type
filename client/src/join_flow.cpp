#include "join_flow.h"

#include <utility>

#include "engine/time/monotonic_time.h"
#include "logging.h"
#include "protocol/error.h"
#include "protocol/message_type.h"
#include "protocol/sequence_tracker.h"

namespace client {

namespace {

constexpr int kMaxJoinAttempts = 5;
constexpr auto kJoinRetryDelay = std::chrono::milliseconds(500);

}  // namespace

JoinFlow::JoinFlow(std::string player_name, std::string room_code)
    : player_name_(std::move(player_name)), room_code_(std::move(room_code)) {
  status_text_ = "Idle";
}

void JoinFlow::Begin(NetworkTransport& transport) {
  state_ = JoinState::kConnecting;
  attempts_ = 0;
  next_sequence_ = 1;
  player_id_.reset();
  last_reject_.reset();
  status_text_ = "Connecting to server";
  sequence_tracker_.Reset();
  SendJoinRequest(transport);
}

void JoinFlow::Update(NetworkTransport& transport) {
  if (state_ == JoinState::kIdle) {
    return;
  }

  if (state_ != JoinState::kConnecting) {
    return;
  }

  engine::net::Client::ReceivedPacket incoming;
  while (transport.Receive(incoming)) {
    protocol::Packet packet;
    protocol::DecodeError error{protocol::DecodeError::kOk};
    if (!protocol::DecodePacket(incoming.buffer, packet, error)) {
      LogPacketError("decode", protocol::DecodeErrorToString(error));
      continue;
    }
    sequence_tracker_.OnRemoteSequenceReceived(packet.header.sequence);
    HandleDecodedPacket(packet);
    if (state_ != JoinState::kConnecting) {
      break;
    }
  }

  if (state_ != JoinState::kConnecting) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (attempts_ < kMaxJoinAttempts && now - last_send_ >= kJoinRetryDelay) {
    SendJoinRequest(transport);
    return;
  }

  if (attempts_ >= kMaxJoinAttempts && now - last_send_ >= kJoinRetryDelay) {
    Fail("Join timed out");
  }
}

void JoinFlow::SendJoinRequest(NetworkTransport& transport) {
  if (!transport.running()) {
    Fail("Network transport not running");
    return;
  }

  protocol::JoinRequestPayload payload;
  payload.client_version = protocol::kProtocolVersion;
  payload.player_name = player_name_;
  payload.room_code = room_code_;

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type = static_cast<std::uint8_t>(
      protocol::message_type::MessageType::kJoinRequest);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = sequence_tracker_.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  // Protocol header uses 32-bit millisecond timestamps; wraparound is accepted
  // by the server for latency measurement.
  packet.header.timestamp_ms =
      static_cast<std::uint32_t>(engine::time::NowMilliseconds());
  sequence_tracker_.FillAckFields(packet.header);
  packet.payload = payload;

  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet, buffer)) {
    Fail("Failed to encode join request");
    return;
  }
  if (!transport.Send(std::move(buffer))) {
    Fail("Failed to queue join request");
    return;
  }

  ++attempts_;
  last_send_ = std::chrono::steady_clock::now();
  status_text_ = "Connecting";
  const auto endpoint = transport.server_endpoint();
  LogConnectionStatus(engine::util::LogLevel::kInfo, endpoint.address(),
                      endpoint.port(), "join request sent");
}

void JoinFlow::HandleDecodedPacket(protocol::Packet& packet) {
  const auto type = static_cast<protocol::message_type::MessageType>(
      packet.header.message_type);
  switch (type) {
    case protocol::message_type::MessageType::kJoinAccept: {
      if (!std::holds_alternative<protocol::JoinAcceptPayload>(
              packet.payload)) {
        LogPacketError("handshake", "Malformed JoinAccept payload");
        return;
      }
      HandleJoinAccept(std::get<protocol::JoinAcceptPayload>(packet.payload));
      break;
    }
    case protocol::message_type::MessageType::kJoinReject: {
      if (!std::holds_alternative<protocol::JoinRejectPayload>(
              packet.payload)) {
        LogPacketError("handshake", "Malformed JoinReject payload");
        return;
      }
      HandleJoinReject(std::get<protocol::JoinRejectPayload>(packet.payload));
      break;
    }
    default:
      break;
  }
}

void JoinFlow::HandleJoinAccept(const protocol::JoinAcceptPayload& payload) {
  player_id_ = payload.player_id;
  last_reject_.reset();
  state_ = JoinState::kConnected;
  status_text_ = "Connected as player " + std::to_string(payload.player_id);
  LogLifecycle(engine::util::LogLevel::kInfo,
               "Join accepted player id " + std::to_string(payload.player_id));
}

void JoinFlow::HandleJoinReject(const protocol::JoinRejectPayload& payload) {
  last_reject_ = payload;
  player_id_.reset();
  state_ = JoinState::kRefused;
  status_text_ = payload.message.empty() ? "Join rejected" : payload.message;
  LogLifecycle(engine::util::LogLevel::kError,
               "Join rejected reason " +
                   std::to_string(static_cast<int>(payload.reason)) + ": " +
                   payload.message);
}

void JoinFlow::MarkDisconnected(std::string_view reason) {
  if (state_ == JoinState::kDisconnected) {
    return;
  }
  player_id_.reset();
  last_reject_.reset();
  state_ = JoinState::kDisconnected;
  status_text_.assign(reason.begin(), reason.end());
  LogLifecycle(engine::util::LogLevel::kWarn, status_text_);
}

void JoinFlow::Reset() {
  state_ = JoinState::kIdle;
  player_id_.reset();
  last_reject_.reset();
  status_text_ = "Idle";
  attempts_ = 0;
  next_sequence_ = 1;
  last_send_ = {};
  sequence_tracker_.Reset();
}

void JoinFlow::Fail(std::string_view message) {
  state_ = JoinState::kRefused;
  status_text_.assign(message.begin(), message.end());
  LogLifecycle(engine::util::LogLevel::kError, status_text_);
}

}  // namespace client
