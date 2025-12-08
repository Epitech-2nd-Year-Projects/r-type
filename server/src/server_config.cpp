#include "server_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
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
constexpr std::uint16_t kMaxTickRate = 1000;

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
                     T* out_value) {
  std::uint64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto [ptr, ec] = std::from_chars(begin, end, parsed);
  if (ec != std::errc() || ptr != end) {
    return false;
  }
  if (parsed < min_value || parsed > max_value) {
    return false;
  }
  *out_value = static_cast<T>(parsed);
  return true;
}

bool TryParseLogLevel(std::string_view value,
                      engine::util::LogLevel* out_level) {
  const auto normalized = Normalize(value);
  constexpr auto kFallback = engine::util::LogLevel::kInfo;
  const auto level = engine::util::ParseLogLevel(normalized, kFallback);
  if (level == kFallback && normalized != "info") {
    return false;
  }
  *out_level = level;
  return true;
}

bool ValidateLength(std::string_view value, std::size_t max_length) {
  return value.size() <= max_length;
}

}  // namespace

ServerConfigParseResult ParseServerConfig(int argc, char** argv) {
  ServerConfig config;
  std::span args(argv, static_cast<std::size_t>(argc));

  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string_view arg(args[i]);
    if (arg == "--port") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --port");
      }
      std::uint16_t port = 0;
      if (!TryParseBounded(args[i + 1], kMinPort, kMaxPort, &port)) {
        return MakeError(config,
                         "Invalid port supplied (must be between 1 and 65535)");
      }
      config.port = port;
      ++i;
      continue;
    }

    if (arg == "--max-players") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --max-players");
      }
      std::uint16_t max_players = 0;
      if (!TryParseBounded(args[i + 1], kMinPlayers, kMaxPlayers,
                           &max_players)) {
        return MakeError(config,
                         "Invalid max players (must be between 1 and 255)");
      }
      config.max_players = max_players;
      ++i;
      continue;
    }

    if (arg == "--tickrate") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --tickrate");
      }
      std::uint16_t tick_rate = 0;
      if (!TryParseBounded(args[i + 1], kMinTickRate, kMaxTickRate,
                           &tick_rate)) {
        return MakeError(config,
                         "Invalid tickrate (must be between 1 and 1000)");
      }
      config.tick_rate = tick_rate;
      ++i;
      continue;
    }

    if (arg == "--room") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --room");
      }
      std::string_view room_value(args[++i]);
      if (room_value.empty()) {
        return MakeError(config, "Room name cannot be empty");
      }
      if (!ValidateLength(room_value, protocol::kMaxRoomCodeLength)) {
        return MakeError(config,
                         "Room name exceeds maximum length of " +
                             std::to_string(protocol::kMaxRoomCodeLength) +
                             " characters");
      }
      config.room_name.assign(room_value);
      continue;
    }

    if (arg == "--seed") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --seed");
      }
      std::uint32_t seed = 0;
      if (!TryParseBounded(args[i + 1],
                           std::numeric_limits<std::uint32_t>::min(),
                           std::numeric_limits<std::uint32_t>::max(), &seed)) {
        return MakeError(config, "Invalid seed value");
      }
      config.seed = seed;
      ++i;
      continue;
    }

    if (arg == "--log-level") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --log-level");
      }
      engine::util::LogLevel level;
      if (!TryParseLogLevel(args[i + 1], &level)) {
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
