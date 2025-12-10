#include "client_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include "engine/util/logging.h"
#include "protocol/join.h"

namespace client {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();
constexpr std::uint32_t kMinTimeoutMs = 3'000;
constexpr std::uint32_t kMaxTimeoutMs = 30'000;

std::string Normalize(std::string_view value) {
  std::string normalized(value.size(), '\0');
  std::transform(
      value.begin(), value.end(), normalized.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return normalized;
}

ClientConfigParseResult MakeError(const ClientConfig& config,
                                  std::string_view message) {
  ClientConfigParseResult result;
  result.config = config;
  result.ok = false;
  result.error.assign(message);
  return result;
}

bool TryParsePort(std::string_view value, std::uint16_t& out_port) {
  unsigned int parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc() || ptr != value.data() + value.size()) {
    return false;
  }
  if (parsed < kMinPort || parsed > kMaxPort) {
    return false;
  }
  out_port = static_cast<std::uint16_t>(parsed);
  return true;
}

bool TryParseLogLevel(std::string_view value,
                      engine::util::LogLevel& out_level) {
  const auto normalized = Normalize(value);
  constexpr auto kFallback = engine::util::LogLevel::kInfo;
  const auto level = engine::util::ParseLogLevel(normalized, kFallback);
  if (level == kFallback && normalized != "info") {
    return false;
  }
  out_level = level;
  return true;
}

bool ValidateLength(std::string_view value, std::size_t max_length) {
  return value.size() <= max_length;
}

bool TryParseTimeout(std::string_view value, std::uint32_t& out_timeout) {
  unsigned int parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc() || ptr != value.data() + value.size()) {
    return false;
  }
  if (parsed < kMinTimeoutMs || parsed > kMaxTimeoutMs) {
    return false;
  }
  out_timeout = static_cast<std::uint32_t>(parsed);
  return true;
}

}  // namespace

ClientConfigParseResult ParseClientConfig(
    std::span<const std::string_view> args) {
  ClientConfig config;

  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string_view arg(args[i]);
    if (arg == "--host") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --host");
      }
      std::string_view host_value(args[++i]);
      if (host_value.empty()) {
        return MakeError(config, "Host cannot be empty");
      }
      config.host = host_value;
      continue;
    }

    if (arg == "--port") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --port");
      }
      std::uint16_t port = 0;
      if (!TryParsePort(args[i + 1], port)) {
        return MakeError(config,
                         "Invalid port supplied (must be between 1 and 65535)");
      }
      config.port = port;
      ++i;
      continue;
    }

    if (arg == "--name") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --name");
      }
      std::string_view name_value(args[++i]);
      if (name_value.empty()) {
        return MakeError(config, "Player name cannot be empty");
      }
      if (!ValidateLength(name_value, protocol::kMaxPlayerNameLength)) {
        return MakeError(config,
                         "Player name exceeds maximum length of " +
                             std::to_string(protocol::kMaxPlayerNameLength) +
                             " characters");
      }
      config.player_name.assign(name_value);
      continue;
    }

    if (arg == "--room") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --room");
      }
      std::string_view room_value(args[++i]);
      if (room_value.empty()) {
        return MakeError(config, "Room code cannot be empty");
      }
      if (!ValidateLength(room_value, protocol::kMaxRoomCodeLength)) {
        return MakeError(config,
                         "Room code exceeds maximum length of " +
                             std::to_string(protocol::kMaxRoomCodeLength) +
                             " characters");
      }
      config.room_code.assign(room_value);
      continue;
    }

    if (arg == "--debug") {
      config.debug = true;
      continue;
    }

    if (arg == "--timeout-ms") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --timeout-ms");
      }
      std::uint32_t timeout = 0;
      if (!TryParseTimeout(args[i + 1], timeout)) {
        return MakeError(
            config,
            "Invalid timeout (must be between 3000 and 30000 milliseconds)");
      }
      config.timeout_ms = timeout;
      ++i;
      continue;
    }

    if (arg == "--log-level") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --log-level");
      }
      engine::util::LogLevel level;
      if (!TryParseLogLevel(args[i + 1], level)) {
        return MakeError(config,
                         "Invalid log level (trace, debug, info, warn/warning, "
                         "error, critical/fatal, off/none)");
      }
      config.log_level = level;
      ++i;
      continue;
    }

    return MakeError(config, "Unknown argument: " + std::string(arg));
  }

  if (config.debug && config.log_level > engine::util::LogLevel::kDebug) {
    config.log_level = engine::util::LogLevel::kDebug;
  }

  return ClientConfigParseResult{config, true, {}};
}

}  // namespace client
