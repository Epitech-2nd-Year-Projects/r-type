#include <array>
#include <asio.hpp>
#include <iostream>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "engine/net/udp_socket.h"
#include "engine/util/logging.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"

namespace {

using protocol::message_type::MessageType;

struct ServerConfig {
  std::uint16_t port{4242};
  std::uint8_t max_players{4};
  std::uint8_t tick_rate{60};
  engine::util::LogLevel log_level{engine::util::LogLevel::kInfo};
};

struct ParseResult {
  ServerConfig config{};
  bool ok{true};
  std::string error;
};

engine::util::Logger& ServerLogger() {
  auto& logger = engine::util::Logger::Default();
  static std::once_flag once;
  std::call_once(once, [&logger]() { logger.SetName("server"); });
  return logger;
}

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

ParseResult ParseServerConfig(int argc, char** argv) {
  ServerConfig config;
  std::span args(argv, static_cast<std::size_t>(argc));

  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string_view arg(args[i]);
    if (arg == "--port") {
      if (i + 1 >= args.size()) {
        return {config, false, "Missing value for --port"};
      }
      std::uint16_t parsed_port = 0;
      unsigned int tmp = 0;
      const auto* begin = args[i + 1];
      const auto* end = begin + std::strlen(args[i + 1]);
      const auto [ptr, ec] = std::from_chars(begin, end, tmp);
      if (ec != std::errc() || ptr != end || tmp == 0 ||
          tmp > std::numeric_limits<std::uint16_t>::max()) {
        return {config, false,
                "Invalid port supplied (must be between 1 and 65535)"};
      }
      parsed_port = static_cast<std::uint16_t>(tmp);
      config.port = parsed_port;
      ++i;
      continue;
    }

    if (arg == "--max-players") {
      if (i + 1 >= args.size()) {
        return {config, false, "Missing value for --max-players"};
      }
      unsigned int parsed_players = 0;
      const auto* begin = args[i + 1];
      const auto* end = begin + std::strlen(args[i + 1]);
      const auto [ptr, ec] = std::from_chars(begin, end, parsed_players);
      if (ec != std::errc() || ptr != end || parsed_players == 0 ||
          parsed_players > std::numeric_limits<std::uint8_t>::max()) {
        return {config, false,
                "Invalid max players (must be between 1 and 255)"};
      }
      config.max_players = static_cast<std::uint8_t>(parsed_players);
      ++i;
      continue;
    }

    if (arg == "--log-level") {
      if (i + 1 >= args.size()) {
        return {config, false, "Missing value for --log-level"};
      }
      const auto level = engine::util::ParseLogLevel(
          args[i + 1], engine::util::LogLevel::kInfo);
      config.log_level = level;
      ++i;
      continue;
    }

    return {config, false, "Unknown argument: " + std::string(arg)};
  }
  return {config, true, {}};
}

class JoinServer {
 public:
  explicit JoinServer(ServerConfig config)
      : config_(config), rng_(std::random_device{}()) {}

  std::error_code Start() {
    auto& logger = ServerLogger();
    logger.SetLevel(config_.log_level);

    const auto bind_endpoint = engine::net::Endpoint::AnyIpv4(config_.port);
    if (auto open_error = socket_.open(engine::net::UdpSocket::Protocol::kIpv4);
        open_error) {
      return open_error;
    }
    if (auto bind_error = socket_.bind(bind_endpoint); bind_error) {
      return bind_error;
    }
    logger.Log(engine::util::LogLevel::kInfo, "Server listening on ",
               bind_endpoint.address(), ":", bind_endpoint.port());
    return {};
  }

  void Run() {
    auto& logger = ServerLogger();
    std::array<std::uint8_t, 2048> buffer{};

    while (true) {
      const auto recv_result =
          socket_.receive_from(buffer.data(), buffer.size());

      if (recv_result.error) {
        if (IsTransientError(recv_result.error)) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }
        logger.Log(engine::util::LogLevel::kError,
                   "Receive error: ", recv_result.error.message());
        break;
      }

      if (recv_result.bytes_transferred == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      engine::net::PacketBuffer packet_buffer(buffer.data(),
                                              recv_result.bytes_transferred);
      HandlePacket(std::move(packet_buffer), recv_result.remote_endpoint);
    }
  }

 private:
  void HandlePacket(engine::net::PacketBuffer packet,
                    const engine::net::Endpoint& from) {
    auto& logger = ServerLogger();
    protocol::Packet decoded{};
    protocol::DecodeError error{protocol::DecodeError::kOk};

    if (!protocol::DecodePacket(packet, decoded, &error)) {
      logger.Log(engine::util::LogLevel::kWarn, "Dropped packet from ",
                 EndpointKey(from), " (", protocol::DecodeErrorToString(error),
                 ")");
      return;
    }

    const auto type = static_cast<MessageType>(decoded.header.message_type);
    if (type != MessageType::kJoinRequest) {
      logger.Log(engine::util::LogLevel::kDebug,
                 "Ignoring non-join packet from ", EndpointKey(from));
      return;
    }

    const auto* request =
        std::get_if<protocol::JoinRequestPayload>(&decoded.payload);
    if (!request) {
      logger.Log(engine::util::LogLevel::kWarn, "Malformed join request from ",
                 EndpointKey(from));
      return;
    }
    ProcessJoin(*request, decoded.header, from);
  }

  void ProcessJoin(const protocol::JoinRequestPayload& request,
                   const protocol::Header& header,
                   const engine::net::Endpoint& from) {
    auto& logger = ServerLogger();
    const auto endpoint_key = EndpointKey(from);

    if (request.client_version != protocol::kProtocolVersion) {
      logger.Log(engine::util::LogLevel::kWarn, "Rejecting join from ",
                 endpoint_key, " due to version mismatch");
      SendReject(protocol::JoinRejectReason::kVersionMismatch,
                 "Protocol version mismatch", header.sequence, from);
      return;
    }

    const auto existing = player_ids_.find(endpoint_key);
    if (existing != player_ids_.end()) {
      SendAccept(existing->second, header.sequence, from);
      return;
    }

    if (player_ids_.size() >= config_.max_players) {
      logger.Log(engine::util::LogLevel::kWarn, "Rejecting join from ",
                 endpoint_key, " because lobby is full");
      SendReject(protocol::JoinRejectReason::kServerFull, "Server is full",
                 header.sequence, from);
      return;
    }

    const std::uint32_t player_id = next_player_id_++;
    player_ids_.emplace(endpoint_key, player_id);

    logger.Log(engine::util::LogLevel::kInfo, "Accepted join from ",
               endpoint_key, " assigned id ", player_id);
    SendAccept(player_id, header.sequence, from);
  }

  void SendAccept(std::uint32_t player_id, std::uint32_t ack_sequence,
                  const engine::net::Endpoint& to) {
    protocol::JoinAcceptPayload payload;
    payload.server_version = protocol::kProtocolVersion;
    payload.player_id = player_id;
    payload.max_players = config_.max_players;
    payload.tick_rate = config_.tick_rate;
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

  void SendReject(protocol::JoinRejectReason reason, std::string_view message,
                  std::uint32_t ack_sequence, const engine::net::Endpoint& to) {
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

  void SendPacket(const protocol::Packet& packet,
                  const engine::net::Endpoint& to) {
    auto& logger = ServerLogger();
    engine::net::PacketBuffer buffer;
    buffer.reserve(128);
    if (!protocol::EncodePacket(packet, buffer)) {
      logger.Log(engine::util::LogLevel::kError, "Failed to encode packet for ",
                 EndpointKey(to));
      return;
    }
    const auto send_result = socket_.send_to(buffer.data(), buffer.size(), to);
    if (send_result.error) {
      logger.Log(engine::util::LogLevel::kError, "Send error to ",
                 EndpointKey(to), ": ", send_result.error.message());
    }
  }

  engine::net::UdpSocket socket_{engine::net::UdpSocket::Protocol::kIpv4};
  ServerConfig config_;
  std::uint32_t next_sequence_{1};
  std::uint32_t next_player_id_{1};
  std::unordered_map<std::string, std::uint32_t> player_ids_;
  std::mt19937 rng_;
};

}  // namespace

int main(int argc, char** argv) {
  const auto parse_result = ParseServerConfig(argc, argv);
  if (!parse_result.ok) {
    std::cerr << parse_result.error << '\n';
    std::cerr << "Usage: " << argv[0]
              << " [--port <port>] [--max-players <n>] [--log-level <level>]\n";
    return 1;
  }

  JoinServer server(parse_result.config);
  if (const auto start_error = server.Start(); start_error) {
    std::cerr << "Failed to start server: " << start_error.message() << '\n';
    return 1;
  }

  server.Run();
  return 0;
}
