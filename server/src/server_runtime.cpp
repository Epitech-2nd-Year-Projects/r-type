#include "server_runtime.h"

#include <array>
#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"

namespace server {

namespace {

using protocol::message_type::MessageType;

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted;
}

std::string EndpointKey(const engine::net::Endpoint& endpoint) {
  std::string key = endpoint.address();
  key.append(":");
  key.append(std::to_string(endpoint.port()));
  return key;
}

std::uint32_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(now).count());
}

}  // namespace

ServerRuntime::ServerRuntime(ServerConfig config)
    : socket_(engine::net::UdpSocket::Protocol::kIpv4),
      config_(std::move(config)),
      frame_timer_(static_cast<float>(config_.tick_rate)),
      rng_(config_.seed) {
  logger_ = &engine::util::Logger::Default();
}

std::error_code ServerRuntime::Start() {
  ConfigureLogging();

  const auto bind_endpoint = engine::net::Endpoint::AnyIpv4(config_.port);
  if (auto open_error = socket_.open(engine::net::UdpSocket::Protocol::kIpv4);
      open_error) {
    return open_error;
  }
  if (auto bind_error = socket_.bind(bind_endpoint); bind_error) {
    return bind_error;
  }

  logger_->Info("Server listening on ", bind_endpoint.address(), ":",
                bind_endpoint.port());
  logger_->Info("Room ", config_.room_name, " max players ",
                config_.max_players, " tickrate ", config_.tick_rate,
                " seed ", config_.seed);
  return {};
}

void ServerRuntime::Run() {
  std::array<std::uint8_t, 2048> buffer{};

  while (true) {
    const auto delta = frame_timer_.tick();
    const auto recv_result =
        socket_.receive_from(buffer.data(), buffer.size());

    if (recv_result.error) {
      if (IsTransientError(recv_result.error)) {
        TickRateSleep(delta);
        continue;
      }
      logger_->Error("Receive error: ", recv_result.error.message());
      break;
    }

    if (recv_result.bytes_transferred > 0) {
      engine::net::PacketBuffer packet_buffer(buffer.data(),
                                              recv_result.bytes_transferred);
      HandlePacket(std::move(packet_buffer), recv_result.remote_endpoint);
    }

    TickRateSleep(delta);
  }
}

void ServerRuntime::ConfigureLogging() {
  logger_->SetName("server");
  logger_->SetLevel(config_.log_level);
}

void ServerRuntime::TickRateSleep(const engine::time::TimeDelta& delta_time) {
  const float target_ms =
      1000.0f / static_cast<float>(config_.tick_rate > 0 ? config_.tick_rate : 1);
  const float delta_ms = delta_time.as_milliseconds();
  if (delta_ms >= target_ms) {
    return;
  }
  const auto sleep_ms =
      std::chrono::milliseconds(static_cast<int>(target_ms - delta_ms));
  if (sleep_ms.count() > 0) {
    std::this_thread::sleep_for(sleep_ms);
  }
}

void ServerRuntime::HandlePacket(engine::net::PacketBuffer packet,
                                 const engine::net::Endpoint& from) {
  protocol::Packet decoded{};
  protocol::DecodeError error{protocol::DecodeError::kOk};

  if (!protocol::DecodePacket(packet, decoded, &error)) {
    logger_->Warn("Dropped packet from ", EndpointKey(from), " (",
                  protocol::DecodeErrorToString(error), ")");
    return;
  }

  const auto type = static_cast<MessageType>(decoded.header.message_type);
  if (type != MessageType::kJoinRequest) {
    logger_->Debug("Ignoring non-join packet from ", EndpointKey(from));
    return;
  }

  const auto* request =
      std::get_if<protocol::JoinRequestPayload>(&decoded.payload);
  if (!request) {
    logger_->Warn("Malformed join request from ", EndpointKey(from));
    return;
  }
  ProcessJoin(*request, decoded.header, from);
}

void ServerRuntime::ProcessJoin(const protocol::JoinRequestPayload& request,
                                const protocol::Header& header,
                                const engine::net::Endpoint& from) {
  const auto endpoint_key = EndpointKey(from);
  logger_->Debug("Join request from ", endpoint_key, " player ",
                 request.player_name, " room ", request.room_code);

  if (request.client_version != protocol::kProtocolVersion) {
    logger_->Warn("Rejecting join from ", endpoint_key,
                  " due to version mismatch");
    SendReject(protocol::JoinRejectReason::kVersionMismatch,
               "Protocol version mismatch", header.sequence, from);
    return;
  }

  if (!config_.room_name.empty() && request.room_code != config_.room_name) {
    logger_->Warn("Rejecting join from ", endpoint_key, " invalid room ",
                  request.room_code);
    SendReject(protocol::JoinRejectReason::kInvalidRoom, "Room unavailable",
               header.sequence, from);
    return;
  }

  const auto existing = player_ids_.find(endpoint_key);
  if (existing != player_ids_.end()) {
    SendAccept(existing->second, header.sequence, from);
    return;
  }

  if (player_ids_.size() >= config_.max_players) {
    logger_->Warn("Rejecting join from ", endpoint_key,
                  " because lobby is full");
    SendReject(protocol::JoinRejectReason::kServerFull, "Server is full",
               header.sequence, from);
    return;
  }

  const std::uint32_t player_id = next_player_id_++;
  player_ids_.emplace(endpoint_key, player_id);

  logger_->Info("Accepted join from ", endpoint_key, " assigned id ",
                player_id);
  SendAccept(player_id, header.sequence, from);
}

void ServerRuntime::SendAccept(std::uint32_t player_id,
                               std::uint32_t ack_sequence,
                               const engine::net::Endpoint& to) {
  protocol::JoinAcceptPayload payload;
  payload.server_version = protocol::kProtocolVersion;
  payload.player_id = player_id;
  payload.max_players = static_cast<std::uint8_t>(config_.max_players);
  payload.tick_rate = static_cast<std::uint8_t>(config_.tick_rate);
  payload.seed = rng_();

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(MessageType::kJoinAccept);
  packet.header.flags =
      static_cast<std::uint8_t>(protocol::HeaderFlag::kHeaderFlagReliable);
  packet.header.sequence = next_sequence_++;
  packet.header.ack = ack_sequence;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  packet.payload = payload;

  SendPacket(packet, to);
}

void ServerRuntime::SendReject(protocol::JoinRejectReason reason,
                               std::string_view message,
                               std::uint32_t ack_sequence,
                               const engine::net::Endpoint& to) {
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
  packet.header.sequence = next_sequence_++;
  packet.header.ack = ack_sequence;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = NowMilliseconds();
  packet.payload = payload;

  SendPacket(packet, to);
}

void ServerRuntime::SendPacket(const protocol::Packet& packet,
                               const engine::net::Endpoint& to) {
  engine::net::PacketBuffer buffer;
  buffer.reserve(128);
  if (!protocol::EncodePacket(packet, buffer)) {
    logger_->Error("Failed to encode packet for ", EndpointKey(to));
    return;
  }
  const auto send_result = socket_.send_to(buffer.data(), buffer.size(), to);
  if (send_result.error) {
    logger_->Error("Send error to ", EndpointKey(to), ": ",
                   send_result.error.message());
  }
}

}  // namespace server
