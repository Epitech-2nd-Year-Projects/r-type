#include "server_runtime.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "protocol/join.h"
#include "protocol/lobby.h"
#include "protocol/message_type.h"
#include "server_runtime_helpers.h"

namespace server {

using namespace runtime_helpers;
using protocol::message_type::MessageType;

namespace {

bool IsValidRoomCode(std::string_view room_code) {
  return room_code.size() <= protocol::kMaxRoomCodeLength;
}

bool IsPrivateRoomCode(std::string_view room_code) {
  if (room_code.size() != 4) {
    return false;
  }
  return std::all_of(room_code.begin(), room_code.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

bool IsValidPlayerName(std::string_view player_name) {
  return player_name.size() <= protocol::kMaxPlayerNameLength;
}

std::optional<std::string> GeneratePrivateCode(
    std::mt19937& rng, const std::unordered_map<std::string, Room>& rooms) {
  std::uniform_int_distribution<int> dist(0, 9999);
  for (int attempt = 0; attempt < 128; ++attempt) {
    const int value = dist(rng);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << value;
    const std::string code = oss.str();
    if (rooms.find(code) == rooms.end()) {
      return code;
    }
  }
  std::vector<std::string> available_codes;
  available_codes.reserve(10'000);
  for (int value = 0; value <= 9999; ++value) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << value;
    const std::string code = oss.str();
    if (rooms.find(code) == rooms.end()) {
      available_codes.push_back(code);
    }
  }
  if (available_codes.empty()) {
    return std::nullopt;
  }
  std::uniform_int_distribution<std::size_t> fallback_dist(
      0, available_codes.size() - 1);
  return available_codes[fallback_dist(rng)];
}

std::string GeneratePublicCode(
    std::mt19937& rng, const std::unordered_map<std::string, Room>& rooms) {
  std::uniform_int_distribution<int> dist(10000, 99999);
  for (int attempt = 0; attempt < 128; ++attempt) {
    const int value = dist(rng);
    std::string code = "room-" + std::to_string(value);
    if (rooms.find(code) == rooms.end()) {
      return code;
    }
  }
  return "room-" + std::to_string(rng());
}

}  // namespace

void ServerRuntime::ProcessJoin(PeerConnection& peer,
                                const protocol::JoinRequestPayload& request) {
  const std::string& endpoint_key = peer.endpoint_key;

  logger_.Debug("Join request from ", endpoint_key, " player ",
                request.player_name, " room ", request.room_code);

  if (!IsValidPlayerName(request.player_name)) {
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " invalid player name length");
    SendReject(peer, protocol::JoinRejectReason::kUnknown,
               "Player name too long");
    return;
  }

  if (request.client_version != protocol::kProtocolVersion) {
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " due to version mismatch");
    SendReject(peer, protocol::JoinRejectReason::kVersionMismatch,
               "Protocol version mismatch");
    return;
  }

  std::string room_code = request.room_code;
  if (room_code.empty()) {
    logger_.Warn("Rejecting join from ", endpoint_key, " missing room code");
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room code required");
    return;
  }
  if (room_code.empty() || !IsValidRoomCode(room_code)) {
    logger_.Warn("Rejecting join from ", endpoint_key, " invalid room code");
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room code too long");
    return;
  }

  auto room = FindRoom(room_code);
  if (!room.has_value()) {
    logger_.Warn("Rejecting join from ", endpoint_key, " unknown room ",
                 room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Room not found");
    return;
  }

  if (room->get().IsPrivate()) {
    if (!IsPrivateRoomCode(request.room_password) ||
        request.room_password != room->get().Password()) {
      logger_.Warn("Rejecting join from ", endpoint_key,
                   " invalid password for room ", room_code);
      SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
                 "Invalid password");
      return;
    }
  }

  if (peer.state == PeerState::kJoined && peer.player_id != 0) {
    if (peer.room_code == room_code) {
      logger_.Debug("Reusing existing player id ", peer.player_id, " for ",
                    endpoint_key, " room ", room_code);
      SendAccept(peer, room_code);
      return;
    }
    logger_.Warn("Rejecting join from ", endpoint_key,
                 " already joined in room ", peer.room_code);
    SendReject(peer, protocol::JoinRejectReason::kInvalidRoom,
               "Already joined");
    return;
  }

  Room& room_ref = room->get();

  peer.player_id = next_player_id_++;
  peer.state = PeerState::kJoined;
  peer.room_code = room_code;
  peer.room_id = room_ref.Id();
  peer.last_seen_ms = NowMilliseconds();
  if (!JoinRoom(peer, room_ref, request.player_name)) {
    peer.player_id = 0;
    peer.state = PeerState::kConnecting;
    peer.room_code.clear();
    peer.room_id = 0;
    SendReject(peer, protocol::JoinRejectReason::kServerFull, "Room full");
    return;
  }
  logger_.Info("Accepted join from ", endpoint_key, " assigned id ",
               peer.player_id, " room ", room_code);
  SendAccept(peer, room_code);
}

void ServerRuntime::HandleRoomListRequest(PeerConnection& peer) {
  protocol::RoomListResponsePayload payload{};
  std::vector<const Room*> public_rooms;
  public_rooms.reserve(rooms_.size());
  for (const auto& [_, room] : rooms_) {
    if (room.IsPrivate()) {
      continue;
    }
    public_rooms.push_back(&room);
  }
  const std::size_t max_entries = protocol::kMaxRoomListEntries;
  const std::size_t limit =
      std::min<std::size_t>(public_rooms.size(), max_entries);
  if (limit > 0) {
    std::partial_sort(public_rooms.begin(), public_rooms.begin() + limit,
                      public_rooms.end(), [](const Room* lhs, const Room* rhs) {
                        return lhs->Name() < rhs->Name();
                      });
  }
  payload.rooms.reserve(limit);
  for (std::size_t i = 0; i < limit; ++i) {
    payload.rooms.push_back(BuildRoomSummary(*public_rooms[i]));
  }

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kRoomListResponse);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = std::move(payload);
  SendPacket(peer, packet);
}

void ServerRuntime::HandleCreateRoomRequest(
    PeerConnection& peer, const protocol::CreateRoomRequestPayload& request) {
  protocol::CreateRoomResponsePayload response{};

  std::uint16_t requested_capacity =
      request.max_players == 0 ? config_.max_players : request.max_players;
  requested_capacity = std::max<std::uint16_t>(1, requested_capacity);
  requested_capacity = std::min<std::uint16_t>(
      requested_capacity, std::numeric_limits<std::uint8_t>::max());

  if (request.room_name.empty()) {
    response.message = "Room name required";
  } else if (request.room_name.size() > protocol::kMaxRoomNameLength) {
    response.message = "Room name too long";
  }

  std::string room_code;
  std::string password;
  if (request.is_private) {
    if (!request.room_password.empty()) {
      if (!IsPrivateRoomCode(request.room_password)) {
        response.message = "Invalid password (must be 4 digits)";
      } else {
        password = request.room_password;
      }
    } else {
      auto generated = GeneratePrivateCode(rng_, rooms_);
      if (!generated.has_value()) {
        response.message = "Unable to allocate a private code";
      } else {
        password = *generated;
      }
    }
    if (response.message.empty()) {
      room_code = "priv-" + password;
    }
  } else if (room_code.empty()) {
    room_code = GeneratePublicCode(rng_, rooms_);
  }

  if (response.message.empty() && !IsValidRoomCode(room_code)) {
    response.message = "Room code too long";
  }
  if (response.message.empty() && rooms_.find(room_code) != rooms_.end()) {
    response.message = "Room already exists";
  }

  if (response.message.empty()) {
    Room& room = CreateRoom(room_code, request.room_name, request.is_private,
                            password, requested_capacity);
    response.success = true;
    response.message = "Room created";
    response.room = BuildRoomSummary(room);
    response.room_password = password;
    logger_.Info("Created room ", room_code,
                 request.is_private ? " (private)" : " (public)", " capacity ",
                 requested_capacity);
  }

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kCreateRoomResponse);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = std::move(response);
  SendPacket(peer, packet);
}

void ServerRuntime::SendAccept(PeerConnection& peer,
                               const std::string& room_code) {
  auto room = FindRoomConst(room_code);
  protocol::JoinAcceptPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.player_id = peer.player_id;
  payload.max_players = static_cast<std::uint8_t>(
      room ? room->get().MaxPlayers() : config_.max_players);
  payload.tick_rate = static_cast<std::uint8_t>(config_.tick_rate);
  payload.seed = room ? room->get().Seed() : rng_();

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kJoinAccept);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);

  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = payload;
  SendPacket(peer, packet);
}

void ServerRuntime::SendReject(PeerConnection& peer,
                               protocol::JoinRejectReason reason,
                               std::string_view message) {
  protocol::JoinRejectPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.reason = reason;
  payload.message.assign(message.begin(), message.end());

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kJoinReject);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = peer.sequence_tracker.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  peer.sequence_tracker.FillAckFields(packet.header);
  packet.payload = payload;
  SendPacket(peer, packet);
}

protocol::RoomSummary ServerRuntime::BuildRoomSummary(const Room& room) const {
  protocol::RoomSummary summary{};
  summary.room_code = room.Code();
  summary.room_name = room.Name();
  summary.is_private = room.IsPrivate();
  summary.max_players = static_cast<std::uint8_t>(std::min<std::uint16_t>(
      room.MaxPlayers(), std::numeric_limits<std::uint8_t>::max()));
  summary.player_count = static_cast<std::uint8_t>(std::min<std::size_t>(
      room.PlayerCount(),
      static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())));
  return summary;
}

void ServerRuntime::EnsureDefaultRoom() {
  if (config_.default_room_code.empty()) {
    return;
  }
  if (rooms_.find(config_.default_room_code) != rooms_.end()) {
    return;
  }
  const std::uint16_t capacity = static_cast<std::uint16_t>(
      std::max<std::uint16_t>(1, config_.max_players));
  Room& room = CreateRoom(config_.default_room_code, config_.default_room_name,
                          /*is_private=*/false, "", capacity);
  logger_.Info("Bootstrapped room ", room.Code(), " capacity ",
               room.MaxPlayers());
}

Room& ServerRuntime::CreateRoom(const std::string& room_code,
                                const std::string& room_name, bool is_private,
                                std::string password,
                                std::uint16_t max_players) {
  const std::uint32_t room_id = next_room_id_++;
  const std::uint32_t seed = rng_();
  auto [inserted, _] = rooms_.emplace(
      room_code, Room{room_code, room_name, room_id, max_players, is_private,
                      std::move(password), seed, logger_});
  inserted->second.MarkActive(NowMilliseconds());
  return inserted->second;
}

bool ServerRuntime::JoinRoom(PeerConnection& peer, Room& room,
                             std::string_view player_name) {
  if (room.PlayerCount() >= room.MaxPlayers()) {
    return false;
  }
  if (!room.AddPlayer(peer.player_id, player_name)) {
    return false;
  }
  players_[peer.player_id] = PlayerSession{peer.endpoint_key, room.Code()};
  room.MarkActive(peer.last_seen_ms);
  return true;
}

std::string ServerRuntime::LeaveRoom(PeerConnection& peer,
                                     std::uint32_t now_ms) {
  std::string room_code = peer.room_code;
  if (peer.player_id == 0) {
    return room_code;
  }
  const auto session = players_.find(peer.player_id);
  if (session != players_.end()) {
    room_code = session->second.room_code;
    players_.erase(session);
  }
  if (auto room = FindRoom(room_code)) {
    if (room->get().RemovePlayer(peer.player_id)) {
      room->get().MarkActive(now_ms);
    }
  }
  return room_code;
}

void ServerRuntime::CleanupRoomIfEmpty(const std::string& room_code,
                                       std::uint32_t now_ms) {
  if (room_code.empty()) {
    return;
  }
  auto it = rooms_.find(room_code);
  if (it == rooms_.end()) {
    return;
  }
  const std::uint32_t last_active = it->second.LastActiveMs();
  const std::uint32_t idle_timeout_ms = config_.room_idle_timeout_ms;
  const bool idle = last_active != 0 && now_ms >= last_active
                        ? now_ms - last_active >= idle_timeout_ms
                        : false;
  if (!it->second.IsEmpty() || !idle) {
    return;
  }
  rooms_.erase(it);
}

}  // namespace server
