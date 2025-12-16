#include "server_runtime.h"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "server_runtime_helpers.h"

namespace server {

using namespace runtime_helpers;

std::optional<std::reference_wrapper<PeerConnection>> ServerRuntime::FindPeer(
    const engine::net::Endpoint& from) {
  const auto endpoint_key = EndpointKey(from);
  const auto it = peers_.find(endpoint_key);
  if (it != peers_.end()) {
    return std::ref(it->second);
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<Room>> ServerRuntime::FindRoom(
    const std::string& room_code) {
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return std::nullopt;
  }
  return std::ref(it->second);
}

std::optional<std::reference_wrapper<const Room>> ServerRuntime::FindRoomConst(
    const std::string& room_code) const {
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return std::nullopt;
  }
  return std::cref(it->second);
}

PeerConnection& ServerRuntime::GetOrCreatePeer(
    const engine::net::Endpoint& endpoint) {
  const auto key = EndpointKey(endpoint);
  auto it = peers_.find(key);
  if (it != peers_.end()) {
    if (!it->second.reliable_queue) {
      it->second.reliable_queue = std::make_unique<protocol::ReliableQueue>(
          kReliableResendTimeoutMs, kReliableQueueMaxPending);
    }
    return it->second;
  }

  PeerConnection peer{};
  peer.endpoint_key = key;
  peer.endpoint = endpoint;
  peer.state = PeerState::kConnecting;
  peer.last_seen_ms = NowMilliseconds();
  peer.reliable_queue = std::make_unique<protocol::ReliableQueue>(
      kReliableResendTimeoutMs, kReliableQueueMaxPending);
  auto [inserted_it, inserted] = peers_.emplace(key, std::move(peer));
  (void)inserted;
  return inserted_it->second;
}

void ServerRuntime::DisconnectPeer(PeerConnection& peer,
                                   std::string_view reason,
                                   bool notify_client) {
  const auto now_ms = NowMilliseconds();
  logger_.Info("Disconnecting peer ", peer.endpoint_key, " player id ",
               peer.player_id, " reason ", reason);
  if (notify_client && peer.state == PeerState::kJoined) {
    SendServerCommand(
        peer,
        static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice),
        reason);
  }
  peer.state = PeerState::kDisconnected;
  std::string room_code = LeaveRoom(peer, now_ms);
  peer.player_id = 0;
  peer.room_code.clear();
  peer.room_id = 0;
  CleanupRoomIfEmpty(room_code, now_ms);
}

void ServerRuntime::CheckPeerTimeouts() {
  const std::uint32_t now_ms = NowMilliseconds();

  for (auto it = peers_.begin(); it != peers_.end();) {
    PeerConnection& peer = it->second;
    const std::uint32_t inactive_ms =
        now_ms >= peer.last_seen_ms
            ? now_ms - peer.last_seen_ms
            : (std::numeric_limits<std::uint32_t>::max() - peer.last_seen_ms) +
                  1u + now_ms;
    if (inactive_ms <= config_.peer_timeout_ms) {
      ++it;
      continue;
    }

    DisconnectPeer(peer, "timeout", peer.state == PeerState::kJoined);
    it = peers_.erase(it);
  }
}

void ServerRuntime::PruneOrphanedSessions() {
  const std::uint32_t now_ms = NowMilliseconds();
  std::vector<std::uint32_t> to_remove;
  to_remove.reserve(players_.size());

  for (const auto& [player_id, session] : players_) {
    auto peer_it = peers_.find(session.endpoint_key);
    const bool missing_peer = peer_it == peers_.end();
    const bool invalid_peer =
        !missing_peer && (peer_it->second.state != PeerState::kJoined ||
                          peer_it->second.player_id != player_id);
    if (missing_peer || invalid_peer) {
      to_remove.push_back(player_id);
    }
  }

  for (std::uint32_t player_id : to_remove) {
    auto session_it = players_.find(player_id);
    if (session_it == players_.end()) {
      continue;
    }
    const std::string room_code = session_it->second.room_code;
    players_.erase(session_it);
    if (auto room = FindRoom(room_code)) {
      if (room->get().RemovePlayer(player_id)) {
        room->get().MarkActive(now_ms);
      }
    }
    CleanupRoomIfEmpty(room_code, now_ms);
  }
}

std::optional<std::reference_wrapper<PeerConnection>>
ServerRuntime::FindPeerByPlayerId(std::uint32_t player_id) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    return std::nullopt;
  }
  auto peer_it = peers_.find(it->second.endpoint_key);
  if (peer_it == peers_.end()) {
    return std::nullopt;
  }
  return std::ref(peer_it->second);
}

void ServerRuntime::RemovePeer(PeerConnection& peer) {
  const std::string endpoint_key = peer.endpoint_key;
  DisconnectPeer(peer, "removed", peer.state == PeerState::kJoined);
  peers_.erase(endpoint_key);
}

}  // namespace server
