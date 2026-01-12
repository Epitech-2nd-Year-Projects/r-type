#include <utility>
#include <vector>

#include "protocol/chat.h"
#include "protocol/message_type.h"
#include "protocol/reliability_policy.h"
#include "server_runtime.h"
#include "server_runtime_helpers.h"

namespace server {

using namespace runtime_helpers;
using protocol::message_type::MessageType;

void ServerRuntime::PollNetwork() {
  const auto poll = transport_.PollNetwork();
  for (auto& datagram : poll.datagrams) {
    HandlePacket(std::move(datagram.payload), datagram.from);
  }
  if (poll.error) {
    logger_.Error("Receive error: ", poll.error.message());
    running_ = false;
  }
}

void ServerRuntime::HandlePacket(engine::net::PacketBuffer packet,
                                 const engine::net::Endpoint& from) {
  engine::net::PacketBuffer reader = packet;
  protocol::Header header;
  if (!protocol::DecodeHeader(reader, header)) {
    return;
  }

  std::optional<engine::net::PacketBuffer> full_packet;

  if (header.flags & protocol::HeaderFlag::kHeaderFlagFragmented) {
    full_packet = reassembler_.HandlePacket(header, reader, EndpointKey(from),
                                            NowMilliseconds());
    if (!full_packet.has_value()) {
      return;
    }
  } else {
    full_packet = std::move(packet);
  }

  protocol::Packet decoded{};
  protocol::DecodeError error{protocol::DecodeError::kOk};

  if (!protocol::DecodePacket(*full_packet, decoded, error)) {
    protocol::UpdateDecodeMetrics(decode_metrics_, error);
    logger_.Warn("Dropped packet from ", EndpointKey(from), " (",
                 protocol::DecodeErrorToString(error),
                 ") total=", decode_metrics_.total_packets,
                 " rejected=", decode_metrics_.rejected_packets);
    return;
  }
  protocol::UpdateDecodeMetrics(decode_metrics_, protocol::DecodeError::kOk);
  PeerConnection& peer = GetOrCreatePeer(from);
  peer.last_seen_ms = NowMilliseconds();
  peer.sequence_tracker.OnRemoteSequenceReceived(decoded.header.sequence);
  ProcessPeerAcks(peer, decoded.header);

  const auto type = static_cast<MessageType>(decoded.header.message_type);

  switch (type) {
    case MessageType::kJoinRequest: {
      if (!std::holds_alternative<protocol::JoinRequestPayload>(
              decoded.payload)) {
        logger_.Warn("Malformed join request from ", peer.endpoint_key);
        return;
      }
      ProcessJoin(peer,
                  std::get<protocol::JoinRequestPayload>(decoded.payload));
      break;
    }
    case MessageType::kRoomListRequest: {
      HandleRoomListRequest(peer);
      break;
    }
    case MessageType::kCreateRoomRequest: {
      if (!std::holds_alternative<protocol::CreateRoomRequestPayload>(
              decoded.payload)) {
        logger_.Warn("Malformed room creation request from ",
                     peer.endpoint_key);
        return;
      }
      HandleCreateRoomRequest(
          peer, std::get<protocol::CreateRoomRequestPayload>(decoded.payload));
      break;
    }
    case MessageType::kPing: {
      if (!std::holds_alternative<protocol::PingPayload>(decoded.payload)) {
        logger_.Warn("Malformed ping from ", peer.endpoint_key);
        return;
      }
      HandlePing(peer, std::get<protocol::PingPayload>(decoded.payload));
      break;
    }
    case MessageType::kInputState: {
      if (!std::holds_alternative<protocol::InputStatePayload>(
              decoded.payload)) {
        logger_.Warn("Malformed input state from ", peer.endpoint_key);
        return;
      }
      HandleInputState(peer,
                       std::get<protocol::InputStatePayload>(decoded.payload),
                       decoded.header);
      break;
    }
    case MessageType::kClientCommand: {
      if (!std::holds_alternative<protocol::CommandPayload>(decoded.payload)) {
        logger_.Warn("Malformed client command from ", peer.endpoint_key);
        return;
      }
      HandleClientCommand(peer,
                          std::get<protocol::CommandPayload>(decoded.payload),
                          decoded.header);
      break;
    }
    case MessageType::kHello:
      logger_.Debug("Hello from ", peer.endpoint_key,
                    " ignored (connectionless)");
      break;
    case MessageType::kPong:
      logger_.Debug("Ignoring connectionless packet (type ",
                    static_cast<int>(type), ") from ", peer.endpoint_key);
      break;
    case MessageType::kServerCommand:
    case MessageType::kWorldSnapshot:
    case MessageType::kSpawnEntity:
    case MessageType::kDestroyEntity:
    case MessageType::kPlayerDied:
      logger_.Warn("Unexpected server-bound packet type ",
                   static_cast<int>(type), " from ", peer.endpoint_key);
      break;
    case MessageType::kInvalid:
      logger_.Warn("Received unexpected packet type ", static_cast<int>(type),
                   " from ", peer.endpoint_key);
      break;
    default:
      logger_.Debug("Ignoring non-join packet (type ", static_cast<int>(type),
                    ") from ", peer.endpoint_key);
      break;
  }
}

void ServerRuntime::HandlePing(PeerConnection& peer,
                               const protocol::PingPayload& ping) {
  protocol::PongPayload pong{};
  pong.client_time_ms = ping.client_time_ms;
  pong.server_time_ms = NowMilliseconds();

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type = static_cast<std::uint8_t>(MessageType::kPong);
  packet.header.flags = 0;
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = pong.server_time_ms;
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = pong;
  SendPacket(peer, packet);
}

void ServerRuntime::HandleInputState(
    PeerConnection& peer, const protocol::InputStatePayload& input_state,
    const protocol::Header& header) {
  if (peer.player_id == 0) {
    logger_.Warn("Received input state from unjoined peer ", peer.endpoint_key);
    return;
  }
  logger_.Trace("InputState from player ", peer.player_id,
                " command_count=", static_cast<int>(input_state.command_count),
                " ack_snap=", input_state.ack_snapshot_id);

  if (input_state.ack_snapshot_id > peer.last_acked_snapshot_id) {
    peer.last_acked_snapshot_id = input_state.ack_snapshot_id;
  }

  for (std::uint8_t i = 0; i < input_state.command_count; ++i) {
    const auto& command = input_state.commands[i];
    logger_.Trace("  Command ", i, ": seq=", command.input_sequence,
                  " buttons=", static_cast<int>(command.buttons),
                  " analog_x=", command.analog_x,
                  " analog_y=", command.analog_y,
                  " client_time_ms=", command.client_time_ms);
  }
  auto room = FindRoom(peer.room_code);
  if (!room.has_value()) {
    logger_.Warn("No game instance for peer ", peer.endpoint_key);
    return;
  }
  room->get().HandleInput(peer.player_id, input_state, header);
  room->get().MarkActive(NowMilliseconds());
}

void ServerRuntime::HandleClientCommand(PeerConnection& peer,
                                        const protocol::CommandPayload& command,
                                        const protocol::Header& header) {
  (void)header;

  if (peer.player_id == 0) {
    logger_.Warn("Received client command from unjoined peer ",
                 peer.endpoint_key);
    return;
  }
  auto room = FindRoom(peer.room_code);
  if (!room.has_value()) {
    logger_.Warn("Client command for unknown room from ", peer.endpoint_key);
    return;
  }

  if (command.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice)) {
    logger_.Info("Peer requested disconnect ", peer.endpoint_key);
    DisconnectPeer(peer, "client disconnect", /*notify_client=*/false);
    peers_.erase(peer.endpoint_key);
    return;
  }

  if (command.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kChatMessage)) {
    if (!protocol::IsValidChatMessage(command.payload)) {
      logger_.Debug("Dropping invalid chat message from player ",
                    peer.player_id);
      return;
    }

    const std::string formatted_message =
        protocol::FormatChatMessage(peer.player_name, command.payload);

    logger_.Debug("Chat from player ", peer.player_id, " (",
                  peer.player_name, "): ", command.payload);

    const auto& players = room->get().Players();
    for (std::uint32_t player_id : players) {
      auto peer_ref = FindPeerByPlayerId(player_id);
      if (!peer_ref.has_value()) {
        continue;
      }
      PeerConnection& target = peer_ref->get();
      if (target.state != PeerState::kJoined || target.player_id == 0 ||
          target.room_code != peer.room_code) {
        continue;
      }
      SendServerCommand(
          target,
          static_cast<std::uint16_t>(protocol::CommandType::kChatMessage),
          formatted_message);
    }
    return;
  }

  logger_.Trace("ClientCommand from player ", peer.player_id,
                " command_id=", command.command_id,
                " data_size=", command.payload.size());

  const auto ready_event =
      room->get().HandleClientCommand(peer.player_id, command);
  room->get().MarkActive(NowMilliseconds());
  if (!ready_event.has_value()) {
    return;
  }

  const std::uint16_t ready_command_id = static_cast<std::uint16_t>(
      ready_event->is_ready ? protocol::CommandType::kSetReady
                            : protocol::CommandType::kUnready);
  const std::string payload = std::to_string(ready_event->player_id);

  const auto& players = room->get().Players();
  for (std::uint32_t player_id : players) {
    auto peer_ref = FindPeerByPlayerId(player_id);
    if (!peer_ref.has_value()) {
      continue;
    }
    PeerConnection& target = peer_ref->get();
    if (target.state != PeerState::kJoined || target.player_id == 0 ||
        target.room_code != peer.room_code) {
      continue;
    }
    SendServerCommand(target, ready_command_id, payload);
    if (ready_event->game_started) {
      SendServerCommand(
          target, static_cast<std::uint16_t>(protocol::CommandType::kStartGame),
          "");
    }
  }
}

void ServerRuntime::SendServerCommand(PeerConnection& peer,
                                      std::uint16_t command_id,
                                      std::string_view payload) {
  protocol::CommandPayload command{};
  command.command_id = command_id;
  command.payload.assign(payload.begin(), payload.end());

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kServerCommand);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.header.timestamp_ms = NowMilliseconds();
  packet.payload = command;
  SendPacket(peer, packet);
}

void ServerRuntime::BroadcastServerCommand(std::uint16_t command_id,
                                           std::string_view payload) {
  for (auto& [_, peer] : peers_) {
    if (peer.state != PeerState::kJoined || peer.player_id == 0) {
      continue;
    }
    SendServerCommand(peer, command_id, payload);
  }
}

void ServerRuntime::SendPacket(PeerConnection& peer,
                               const protocol::Packet& packet) {
  protocol::Packet packet_to_send = packet;
  const auto type =
      static_cast<MessageType>(packet_to_send.header.message_type);
  const bool reliable_by_policy = protocol::IsReliable(type);
  if (reliable_by_policy) {
    packet_to_send.header.flags |=
        static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  }

  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet_to_send, buffer)) {
    logger_.Error("Failed to encode packet for ", peer.endpoint_key);
    return;
  }

  const bool is_reliable =
      reliable_by_policy ||
      ((packet_to_send.header.flags &
        static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable)) !=
       0);

  if (is_reliable && peer.reliable_queue) {
    peer.reliable_queue->AddSentPacket(packet_to_send.header.sequence,
                                       buffer.storage(), NowMilliseconds());
  }

  auto fragments = protocol::SplitPacketBuffer(buffer);
  for (auto& frag : fragments) {
    const auto send_result = transport_.Send(peer.endpoint, frag);
    if (send_result.error) {
      logger_.Error("Send error to ", peer.endpoint_key, ": ",
                    send_result.error.message());
    }
  }
}

void ServerRuntime::ProcessPeerAcks(PeerConnection& peer,
                                    const protocol::Header& header) {
  if (peer.reliable_queue == nullptr) {
    return;
  }
  peer.reliable_queue->OnAckReceived(header.ack, header.ack_bits);
}

void ServerRuntime::ProcessReliableResends() {
  const std::uint32_t now_ms = NowMilliseconds();

  for (auto& [_, peer] : peers_) {
    if (!peer.reliable_queue) {
      continue;
    }
    std::vector<protocol::PendingPacket> to_resend;
    peer.reliable_queue->CollectPacketsToResend(now_ms, to_resend);
    for (const auto& pending : to_resend) {
      engine::net::PacketBuffer pending_buffer;

      pending_buffer.write_bytes(std::span<const std::uint8_t>(pending.bytes));

      auto fragments = protocol::SplitPacketBuffer(pending_buffer);
      bool any_error = false;
      for (auto& frag : fragments) {
        const auto send_result = transport_.Send(peer.endpoint, frag);
        if (send_result.error) {
          logger_.Warn("Resend error to ", peer.endpoint_key, ": ",
                       send_result.error.message());
          any_error = true;
        }
      }
      if (any_error) {
        peer.reliable_queue->MarkSendFailed(pending.sequence, now_ms);
      }
    }
  }
}

}  // namespace server
