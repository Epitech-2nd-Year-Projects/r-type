#include "room_directory_client.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "engine/time/monotonic_time.h"
#include "protocol/error.h"

namespace client {

namespace {
constexpr int kMaxAttempts = 4;
constexpr auto kRetryDelay = std::chrono::milliseconds(400);
}  // namespace

RoomDirectoryClient::RoomDirectoryClient(
    std::shared_ptr<NetworkTransport> transport)
    : transport_(std::move(transport)),
      logger_(engine::util::Logger::Default()) {}

bool RoomDirectoryClient::Connect(std::string host, std::uint16_t port) {
  host_ = std::move(host);
  port_ = port;

  if (transport_->running()) {
    const auto endpoint = transport_->server_endpoint();
    const bool same_target =
        endpoint.port() == port_ && endpoint.address() == host_;
    if (same_target) {
      return true;
    }
    transport_->Stop();
  }

  const auto error = transport_->Start(host_, port_);
  if (error) {
    status_text_ = "Failed to reach server: " + error.message();
    logger_.Warn(status_text_);
    return false;
  }
  status_text_ = "Connected to lobby";
  sequence_tracker_.Reset();
  return true;
}

void RoomDirectoryClient::Disconnect() {
  transport_->Stop();
  sequence_tracker_.Reset();
  ResetPending();
}

void RoomDirectoryClient::RequestRoomList() {
  pending_create_.reset();
  pending_list_ = protocol::RoomListRequestPayload{};
  active_operation_ = Operation::kList;
  attempts_ = 0;
  last_send_ = {};
  status_text_ = "Requesting room list...";
  EnsureTransport();
}

void RoomDirectoryClient::RequestCreateRoom(const std::string& room_name,
                                            bool is_private,
                                            std::string room_password,
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
  status_text_ = "Creating room...";
  EnsureTransport();
}

void RoomDirectoryClient::Update() {
  if (!transport_ || !transport_->running()) {
    return;
  }

  engine::net::Client::ReceivedPacket incoming;
  while (transport_->Receive(incoming)) {
    protocol::Packet packet{};
    protocol::DecodeError error{protocol::DecodeError::kOk};
    if (!protocol::DecodePacket(incoming.buffer, packet, error)) {
      logger_.Warn("Dropped lobby packet: ",
                   protocol::DecodeErrorToString(error));
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
                        now - last_send_ >= kRetryDelay;
  if (can_send && attempts_ < kMaxAttempts) {
    if (pending_list_) {
      SendPacket(protocol::message_type::MessageType::kRoomListRequest,
                 *pending_list_);
    } else if (pending_create_) {
      SendPacket(protocol::message_type::MessageType::kCreateRoomRequest,
                 *pending_create_);
    }
    last_send_ = now;
    ++attempts_;
  } else if (attempts_ >= kMaxAttempts && can_send) {
    status_text_ = "Lobby request timed out";
    ResetPending();
  }
}

std::optional<protocol::CreateRoomResponsePayload>
RoomDirectoryClient::ConsumeCreateResponse() {
  auto result = std::move(last_create_response_);
  last_create_response_.reset();
  return result;
}

void RoomDirectoryClient::SendPacket(protocol::message_type::MessageType type,
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
    status_text_ = "Failed to encode lobby packet";
    return;
  }
  transport_->Send(std::move(buffer));
}

void RoomDirectoryClient::HandlePacket(protocol::Packet& packet) {
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

void RoomDirectoryClient::HandleListResponse(
    const protocol::RoomListResponsePayload& payload) {
  rooms_ = payload.rooms;
  status_text_ = rooms_.empty()
                     ? "No rooms available"
                     : "Received " + std::to_string(rooms_.size()) + " rooms";
  ResetPending();
}

void RoomDirectoryClient::HandleCreateRoomResponse(
    const protocol::CreateRoomResponsePayload& payload) {
  last_create_response_ = payload;
  status_text_ = payload.message.empty() ? "Room creation response received"
                                         : payload.message;
  if (payload.success && payload.room && payload.room->is_private &&
      !payload.room_password.empty()) {
    status_text_ = "Private room ready. Password: " + payload.room_password;
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
  }
  ResetPending();
}

bool RoomDirectoryClient::EnsureTransport() {
  if (transport_ && transport_->running()) {
    return true;
  }
  if (host_.empty() || port_ == 0) {
    status_text_ = "Lobby target not configured";
    return false;
  }
  return Connect(host_, port_);
}

void RoomDirectoryClient::ResetPending() {
  pending_list_.reset();
  pending_create_.reset();
  active_operation_ = Operation::kNone;
  attempts_ = 0;
  last_send_ = {};
}

}  // namespace client
