#include "lobby_service.h"

#include <algorithm>
#include <limits>
#include <system_error>
#include <utility>

#include "engine/net/endpoint.h"
#include "engine/time/monotonic_time.h"
#include "logging.h"
#include "protocol/error.h"

namespace client {

namespace {

LobbyRetryPolicy NormalizeRetryPolicy(LobbyRetryPolicy policy) {
  if (policy.max_attempts < 1) {
    policy.max_attempts = 1;
  }
  if (policy.retry_delay <= std::chrono::milliseconds(0)) {
    policy.retry_delay = std::chrono::milliseconds(1);
  }
  return policy;
}

}  // namespace

LobbyService::LobbyService(std::shared_ptr<NetworkTransport> transport,
                           LobbyRetryPolicy retry_policy,
                           LobbyCallbacks callbacks)
    : transport_(std::move(transport)),
      retry_policy_(NormalizeRetryPolicy(retry_policy)),
      callbacks_(std::move(callbacks)) {}

bool LobbyService::Connect(std::string host, std::uint16_t port) {
  host_ = std::move(host);
  port_ = port;

  if (!transport_) {
    SetStatus("Lobby transport unavailable");
    LogLobby(engine::util::LogLevel::kError, status_text_);
    return false;
  }

  if (transport_->running()) {
    const auto endpoint = transport_->server_endpoint();
    std::error_code resolve_error;
    const auto resolved =
        engine::net::Endpoint::Resolve(host_, port_, resolve_error);
    bool same_target = false;
    if (!resolve_error && resolved.valid()) {
      same_target = endpoint.port() == resolved.port() &&
                    endpoint.native().address() == resolved.native().address();
    } else {
      same_target = endpoint.port() == port_ && endpoint.address() == host_;
    }
    if (same_target) {
      return true;
    }
    transport_->Stop();
  }

  const auto error = transport_->Start(host_, port_);
  if (error) {
    SetStatus("Failed to reach server: " + error.message());
    LogLobby(engine::util::LogLevel::kWarn, status_text_);
    return false;
  }

  SetStatus("Connected to lobby");
  sequence_tracker_.Reset();
  return true;
}

void LobbyService::Disconnect() {
  if (transport_) {
    transport_->Stop();
  }
  sequence_tracker_.Reset();
  ResetPending();
  SetStatus("Lobby idle");
}

void LobbyService::RequestRoomList() {
  pending_create_.reset();
  pending_list_ = protocol::RoomListRequestPayload{};
  active_operation_ = Operation::kList;
  attempts_ = 0;
  last_send_ = {};
  SetStatus("Requesting room list...");
  EnsureTransport();
}

void LobbyService::RequestCreateRoom(const std::string& room_name,
                                     bool is_private, std::string room_password,
                                     std::uint16_t max_players) {
  pending_list_.reset();
  protocol::CreateRoomRequestPayload request{};
  request.room_name = room_name;
  request.is_private = is_private;
  request.room_password = std::move(room_password);
  request.max_players = static_cast<std::uint8_t>(std::min<std::uint16_t>(
      max_players, std::numeric_limits<std::uint8_t>::max()));
  pending_create_ = request;
  active_operation_ = Operation::kCreate;
  attempts_ = 0;
  last_send_ = {};
  SetStatus("Creating room...");
  EnsureTransport();
}

void LobbyService::Update() {
  if (!transport_ || !transport_->running()) {
    return;
  }

  engine::net::Client::ReceivedPacket incoming;
  while (transport_->Receive(incoming)) {
    protocol::Packet packet{};
    protocol::DecodeError error{protocol::DecodeError::kOk};
    if (!protocol::DecodePacket(incoming.buffer, packet, error)) {
      LogLobby(engine::util::LogLevel::kWarn,
               std::string("Dropped lobby packet: ") +
                   std::string(protocol::DecodeErrorToString(error)));
      continue;
    }
    sequence_tracker_.OnRemoteSequenceReceived(packet.header.sequence);
    HandlePacket(packet);
  }

  if (active_operation_ == Operation::kNone) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  const bool can_send = last_send_.time_since_epoch().count() == 0 ||
                        now - last_send_ >= retry_policy_.retry_delay;
  if (can_send && attempts_ < retry_policy_.max_attempts) {
    if (pending_list_) {
      SendPacket(protocol::message_type::MessageType::kRoomListRequest,
                 *pending_list_);
    } else if (pending_create_) {
      SendPacket(protocol::message_type::MessageType::kCreateRoomRequest,
                 *pending_create_);
    }
    last_send_ = now;
    ++attempts_;
  } else if (attempts_ >= retry_policy_.max_attempts && can_send) {
    SetStatus("Lobby request timed out");
    ResetPending();
  }
}

std::optional<protocol::CreateRoomResponsePayload>
LobbyService::ConsumeCreateResponse() {
  auto result = std::move(last_create_response_);
  last_create_response_.reset();
  return result;
}

void LobbyService::SendPacket(protocol::message_type::MessageType type,
                              protocol::PacketPayload payload) {
  if (!EnsureTransport()) {
    return;
  }

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type = static_cast<std::uint8_t>(type);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = sequence_tracker_.NextLocalSequence();
  packet.header.timestamp_ms =
      static_cast<std::uint32_t>(engine::time::NowMilliseconds());
  sequence_tracker_.FillAckFields(packet.header);
  packet.payload = std::move(payload);

  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet, buffer)) {
    SetStatus("Failed to encode lobby packet");
    return;
  }
  transport_->Send(std::move(buffer));
}

void LobbyService::HandlePacket(protocol::Packet& packet) {
  using protocol::message_type::MessageType;
  const auto type = static_cast<MessageType>(packet.header.message_type);
  switch (type) {
    case MessageType::kRoomListResponse: {
      if (const auto payload =
              std::get_if<protocol::RoomListResponsePayload>(&packet.payload)) {
        HandleListResponse(*payload);
      }
      break;
    }
    case MessageType::kCreateRoomResponse: {
      if (const auto payload = std::get_if<protocol::CreateRoomResponsePayload>(
              &packet.payload)) {
        HandleCreateRoomResponse(*payload);
      }
      break;
    }
    default:
      break;
  }
}

void LobbyService::HandleListResponse(
    const protocol::RoomListResponsePayload& payload) {
  rooms_ = payload.rooms;
  SetStatus(rooms_.empty()
                ? "No rooms available"
                : "Received " + std::to_string(rooms_.size()) + " rooms");
  NotifyRoomsUpdated();
  ResetPending();
}

void LobbyService::HandleCreateRoomResponse(
    const protocol::CreateRoomResponsePayload& payload) {
  last_create_response_ = payload;
  SetStatus(payload.message.empty() ? "Room creation response received"
                                    : payload.message);
  if (payload.success && payload.room && payload.room->is_private &&
      !payload.room_password.empty()) {
    SetStatus("Private room ready. Password: " + payload.room_password);
  }
  if (payload.room) {
    const auto& created = *payload.room;
    auto it = std::find_if(rooms_.begin(), rooms_.end(),
                           [&](const protocol::RoomSummary& summary) {
                             return summary.room_code == created.room_code;
                           });
    if (it == rooms_.end()) {
      rooms_.push_back(created);
    } else {
      *it = created;
    }
    NotifyRoomsUpdated();
  }
  if (callbacks_.room_created) {
    callbacks_.room_created(payload);
  }
  ResetPending();
}

bool LobbyService::EnsureTransport() {
  if (transport_ && transport_->running()) {
    return true;
  }
  if (host_.empty() || port_ == 0) {
    SetStatus("Lobby target not configured");
    return false;
  }
  return Connect(host_, port_);
}

void LobbyService::ResetPending() {
  pending_list_.reset();
  pending_create_.reset();
  active_operation_ = Operation::kNone;
  attempts_ = 0;
  last_send_ = {};
}

void LobbyService::SetStatus(std::string message) {
  if (message == status_text_) {
    return;
  }
  status_text_ = std::move(message);
  if (callbacks_.status_changed) {
    callbacks_.status_changed(status_text_);
  }
}

void LobbyService::NotifyRoomsUpdated() {
  if (callbacks_.rooms_updated) {
    callbacks_.rooms_updated(rooms_);
  }
}

}  // namespace client
