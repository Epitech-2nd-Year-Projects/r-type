#include "server_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <optional>
#include <span>
#include <string_view>

#include "protocol/join.h"

namespace server {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();
constexpr std::uint16_t kMinPlayers = 1;
constexpr std::uint16_t kMaxPlayers = std::numeric_limits<std::uint8_t>::max();
constexpr std::uint16_t kMinTickRate = 1;
constexpr std::uint16_t kMaxTickRate = std::numeric_limits<std::uint8_t>::max();
constexpr std::uint32_t kMinPeerTimeoutMs = 10'000;
constexpr std::uint32_t kMaxPeerTimeoutMs = 30'000;
constexpr std::uint32_t kMinRoomIdleTimeoutMs = 1'000;
constexpr std::uint32_t kMaxRoomIdleTimeoutMs = 600'000;

std::string Normalize(std::string_view value) {
  std::string normalized(value.size(), '\0');
  std::transform(
      value.begin(), value.end(), normalized.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return normalized;
}

ServerConfigParseResult MakeError(const ServerConfig& config,
                                  std::string_view message) {
  ServerConfigParseResult result;
  result.config = config;
  result.ok = false;
  result.error.assign(message);
  return result;
}

template <typename T>
bool TryParseBounded(std::string_view value, T min_value, T max_value,
                     T& out_value) {
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc() || ptr != value.data() + value.size()) {
    return false;
  }
  if (parsed < min_value || parsed > max_value) {
    return false;
  }
  out_value = static_cast<T>(parsed);
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

}  // namespace

ServerConfigParseResult ParseServerConfig(
    std::span<const std::string_view> args) {
  ServerConfig config;
  auto missing_value_error = [&](std::string_view flag) {
    return MakeError(config, "Missing value for " + std::string(flag));
  };
  auto next_value = [&](std::size_t index) -> std::optional<std::string_view> {
    if (index + 1 >= args.size()) {
      return std::nullopt;
    }
    return args[index + 1];
  };

  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string_view arg(args[i]);
    if (arg == "--port") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--port");
      }
      std::uint16_t port = 0;
      if (!TryParseBounded(*value, kMinPort, kMaxPort, port)) {
        return MakeError(config,
                         "Invalid port supplied (must be between 1 and 65535)");
      }
      config.port = port;
      ++i;
      continue;
    }

    if (arg == "--max-players") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--max-players");
      }
      std::uint16_t max_players = 0;
      if (!TryParseBounded(*value, kMinPlayers, kMaxPlayers, max_players)) {
        return MakeError(config,
                         "Invalid max players (must be between 1 and 255)");
      }
      config.max_players = max_players;
      ++i;
      continue;
    }

    if (arg == "--tickrate" || arg == "--tick-rate") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error(arg);
      }
      std::uint16_t tick_rate = 0;
      if (!TryParseBounded(*value, kMinTickRate, kMaxTickRate, tick_rate)) {
        return MakeError(config,
                         "Invalid tickrate (must be between 1 and 255)");
      }
      config.tick_rate = tick_rate;
      ++i;
      continue;
    }

    if (arg == "--timeout-ms") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--timeout-ms");
      }
      std::uint32_t timeout_ms = 0;
      if (!TryParseBounded(*value, kMinPeerTimeoutMs, kMaxPeerTimeoutMs,
                           timeout_ms)) {
        return MakeError(
            config, "Invalid timeout (must be between 10000 and 30000 ms)");
      }
      config.peer_timeout_ms = timeout_ms;
      ++i;
      continue;
    }

    if (arg == "--room-idle-timeout-ms") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--room-idle-timeout-ms");
      }
      std::uint32_t idle_ms = 0;
      if (!TryParseBounded(*value, kMinRoomIdleTimeoutMs, kMaxRoomIdleTimeoutMs,
                           idle_ms)) {
        return MakeError(
            config,
            "Invalid room idle timeout (must be between 1000 and 600000 ms)");
      }
      config.room_idle_timeout_ms = idle_ms;
      ++i;
      continue;
    }

    if (arg == "--room" || arg == "--room-code") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error(arg);
      }
      std::string_view room_value(*value);
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
      ++i;
      continue;
    }

    if (arg == "--seed") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--seed");
      }
      std::uint32_t seed = 0;
      if (!TryParseBounded(*value, std::numeric_limits<std::uint32_t>::min(),
                           std::numeric_limits<std::uint32_t>::max(), seed)) {
        return MakeError(config, "Invalid seed value");
      }
      config.seed = seed;
      ++i;
      continue;
    }

    if (arg == "--log-level") {
      const auto value = next_value(i);
      if (!value) {
        return missing_value_error("--log-level");
      }
      engine::util::LogLevel level;
      if (!TryParseLogLevel(*value, level)) {
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

  return ServerConfigParseResult{config, true, {}};
}

}  // namespace server
