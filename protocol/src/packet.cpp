#include "protocol/packet.h"

#include <utility>

#include "protocol/message_type.h"

namespace protocol {

namespace {

using message_type::MessageType;

bool EncodePayloadByType(const PacketPayload& payload, MessageType type,
                         engine::net::PacketBuffer& buffer) {
  switch (type) {
    case MessageType::kInputState: {
      if (!std::holds_alternative<InputStatePayload>(payload)) {
        return false;
      }
      const auto& value = std::get<InputStatePayload>(payload);
      return EncodeInputState(value, buffer);
    }
    case MessageType::kPing: {
      if (!std::holds_alternative<PingPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<PingPayload>(payload);
      return EncodePing(value, buffer);
    }
    case MessageType::kPong: {
      if (!std::holds_alternative<PongPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<PongPayload>(payload);
      return EncodePong(value, buffer);
    }
    case MessageType::kJoinRequest: {
      if (!std::holds_alternative<JoinRequestPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<JoinRequestPayload>(payload);
      return EncodeJoinRequest(value, buffer);
    }
    case MessageType::kJoinAccept: {
      if (!std::holds_alternative<JoinAcceptPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<JoinAcceptPayload>(payload);
      return EncodeJoinAccept(value, buffer);
    }
    case MessageType::kJoinReject: {
      if (!std::holds_alternative<JoinRejectPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<JoinRejectPayload>(payload);
      return EncodeJoinReject(value, buffer);
    }
    case MessageType::kPlayerDied: {
      if (!std::holds_alternative<PlayerDiedPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<PlayerDiedPayload>(payload);
      return EncodePlayerDied(value, buffer);
    }
    case MessageType::kWorldSnapshot: {
      if (!std::holds_alternative<WorldSnapshotPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<WorldSnapshotPayload>(payload);
      return EncodeWorldSnapshot(value, buffer);
    }
    case MessageType::kClientCommand:
    case MessageType::kServerCommand: {
      if (!std::holds_alternative<CommandPayload>(payload)) {
        return false;
      }
      const auto& value = std::get<CommandPayload>(payload);
      return EncodeCommand(value, buffer);
    }
    default:
      return false;
  }
}

bool DecodePayloadByType(engine::net::PacketBuffer& buffer, MessageType type,
                         PacketPayload& out_payload) {
  switch (type) {
    case MessageType::kInputState: {
      InputStatePayload value;
      if (!DecodeInputState(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kPing: {
      PingPayload value;
      if (!DecodePing(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kPong: {
      PongPayload value;
      if (!DecodePong(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kJoinRequest: {
      JoinRequestPayload value;
      if (!DecodeJoinRequest(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kJoinAccept: {
      JoinAcceptPayload value;
      if (!DecodeJoinAccept(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kJoinReject: {
      JoinRejectPayload value;
      if (!DecodeJoinReject(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kPlayerDied: {
      PlayerDiedPayload value;
      if (!DecodePlayerDied(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kWorldSnapshot: {
      WorldSnapshotPayload value;
      if (!DecodeWorldSnapshot(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    case MessageType::kClientCommand:
    case MessageType::kServerCommand: {
      CommandPayload value;
      if (!DecodeCommand(buffer, value)) {
        return false;
      }
      out_payload = std::move(value);
      return true;
    }
    default:
      return false;
  }
}

bool DecodePacketInternal(engine::net::PacketBuffer& buffer, Packet& out_packet,
                          DecodeError* out_error) {
  if (out_error != nullptr) {
    *out_error = DecodeError::kOk;
  }

  Header header{};
  if (!DecodeHeader(buffer, header)) {
    if (out_error != nullptr) {
      *out_error = DecodeError::kUnexpectedEndOfBuffer;
    }
    return false;
  }

  if (header.version != kProtocolVersion) {
    if (out_error != nullptr) {
      *out_error = DecodeError::kVersionMismatch;
    }
    return false;
  }

  const MessageType type = static_cast<MessageType>(header.message_type);

  switch (type) {
    case MessageType::kHello:
    case MessageType::kJoinRequest:
    case MessageType::kJoinAccept:
    case MessageType::kJoinReject:
    case MessageType::kInputState:
    case MessageType::kWorldSnapshot:
    case MessageType::kSpawnEntity:
    case MessageType::kDestroyEntity:
    case MessageType::kPlayerDied:
    case MessageType::kClientCommand:
    case MessageType::kServerCommand:
    case MessageType::kPing:
    case MessageType::kPong:
      break;
    case MessageType::kInvalid:
    default:
      if (out_error != nullptr) {
        *out_error = DecodeError::kUnknownMessageType;
      }
      return false;
  }

  PacketPayload payload;
  if (!DecodePayloadByType(buffer, type, payload)) {
    if (out_error != nullptr) {
      *out_error = DecodeError::kInvalidPayload;
    }
    return false;
  }

  out_packet.header = header;
  out_packet.payload = std::move(payload);
  return true;
}

}  // namespace

bool EncodePacket(const Packet& packet, engine::net::PacketBuffer& buffer) {
  const MessageType type = static_cast<MessageType>(packet.header.message_type);

  EncodeHeader(packet.header, buffer);
  return EncodePayloadByType(packet.payload, type, buffer);
}

bool DecodePacket(engine::net::PacketBuffer& buffer, Packet& out_packet) {
  return DecodePacketInternal(buffer, out_packet, nullptr);
}

bool DecodePacket(engine::net::PacketBuffer& buffer, Packet& out_packet,
                  DecodeError* out_error) {
  return DecodePacketInternal(buffer, out_packet, out_error);
}

}  // namespace protocol
