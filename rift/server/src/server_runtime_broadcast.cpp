#include <string>
#include <vector>

#include "protocol/command.h"
#include "protocol/message_type.h"
#include "server_runtime.h"
#include "server_runtime_helpers.h"

namespace server {

using namespace runtime_helpers;
using protocol::message_type::MessageType;

void ServerRuntime::BroadcastWorldSnapshots() {
  std::vector<std::string> empty_rooms;
  const auto now_ms = NowMilliseconds();

  for (auto& [room_code, room] : rooms_) {
    if (room.IsEmpty()) {
      empty_rooms.push_back(room_code);
      continue;
    }

    protocol::WorldSnapshotPayload full_snapshot =
        room.BuildSnapshot(server_tick_);
    room.MarkActive(now_ms);

    for (std::uint32_t player_id : room.Players()) {
      auto peer_ref = FindPeerByPlayerId(player_id);
      if (!peer_ref.has_value()) {
        continue;
      }
      PeerConnection& peer = peer_ref->get();
      if (peer.state != PeerState::kJoined || peer.player_id == 0 ||
          peer.room_code != room_code) {
        continue;
      }

      protocol::Packet packet{};
      packet.header.version = protocol::kProtocolVersion;
      packet.header.message_type =
          static_cast<std::uint8_t>(MessageType::kWorldSnapshot);
      packet.header.flags = 0;
      packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
      packet.header.timestamp_ms = now_ms;
      peer.sequence_tracker.FillAckFields(packet.header);

      const auto base_snapshot = room.GetSnapshot(peer.last_acked_snapshot_id);
      if (base_snapshot.has_value()) {
        packet.payload =
            protocol::ComputeDelta(full_snapshot, base_snapshot->get());
      } else {
        packet.payload = full_snapshot;
      }

      SendPacket(peer, packet);
    }
  }

  for (const auto& room_code : empty_rooms) {
    CleanupRoomIfEmpty(room_code, now_ms);
  }
}

void ServerRuntime::BroadcastGameEvents() {
  const std::uint32_t now_ms = NowMilliseconds();
  for (auto& [room_code, room] : rooms_) {
    const auto deaths = room.PollPlayerDeaths();
    for (const auto& payload : deaths) {
      for (std::uint32_t player_id : room.Players()) {
        auto peer_ref = FindPeerByPlayerId(player_id);
        if (!peer_ref.has_value()) {
          continue;
        }
        PeerConnection& peer = peer_ref->get();
        if (peer.state != PeerState::kJoined || peer.room_code != room_code) {
          continue;
        }

        protocol::Packet packet{};
        packet.header.version = protocol::kProtocolVersion;
        packet.header.message_type =
            static_cast<std::uint8_t>(MessageType::kPlayerDied);
        packet.header.flags = static_cast<std::uint8_t>(
            protocol::HeaderFlag::kHeaderFlagReliable);
        packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
        packet.header.timestamp_ms = now_ms;
        peer.sequence_tracker.FillAckFields(packet.header);
        packet.payload = payload;
        SendPacket(peer, packet);
      }
    }

    if (room.PollMatchOver()) {
      for (std::uint32_t player_id : room.Players()) {
        auto peer_ref = FindPeerByPlayerId(player_id);
        if (!peer_ref.has_value()) {
          continue;
        }
        PeerConnection& peer = peer_ref->get();
        if (peer.state != PeerState::kJoined || peer.room_code != room_code) {
          continue;
        }

        protocol::CommandPayload cmd{};
        cmd.command_id =
            static_cast<std::uint16_t>(protocol::CommandType::kMatchOver);

        protocol::Packet packet{};
        packet.header.version = protocol::kProtocolVersion;
        packet.header.message_type =
            static_cast<std::uint8_t>(MessageType::kServerCommand);
        packet.header.flags = static_cast<std::uint8_t>(
            protocol::HeaderFlag::kHeaderFlagReliable);
        packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
        packet.header.timestamp_ms = now_ms;
        peer.sequence_tracker.FillAckFields(packet.header);
        packet.payload = cmd;
        SendPacket(peer, packet);
      }
    }
  }
}

}  // namespace server
