#include "server_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <type_traits>

#include "protocol/join.h"
#include "protocol/lobby.h"

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

ServerConfig LoadServerConfig() {
  ServerConfig config;

  const auto apply_uint = [&](const char* env_name, auto min_value,
                              auto max_value, auto& target) {
    const char* value = std::getenv(env_name);
    if (!value) return;
    typename std::remove_reference_t<decltype(target)> parsed{};
    if (TryParseBounded(std::string_view(value), min_value, max_value,
                        parsed)) {
      target = parsed;
    }
  };

  apply_uint("RTYPE_SERVER_PORT", kMinPort, kMaxPort, config.port);
  apply_uint("RTYPE_SERVER_MAX_PLAYERS", kMinPlayers, kMaxPlayers,
             config.max_players);
  apply_uint("RTYPE_SERVER_TICK_RATE", kMinTickRate, kMaxTickRate,
             config.tick_rate);
  apply_uint("RTYPE_SERVER_TIMEOUT_MS", kMinPeerTimeoutMs, kMaxPeerTimeoutMs,
             config.peer_timeout_ms);
  apply_uint("RTYPE_SERVER_ROOM_IDLE_TIMEOUT_MS", kMinRoomIdleTimeoutMs,
             kMaxRoomIdleTimeoutMs, config.room_idle_timeout_ms);

  if (const char* seed_value = std::getenv("RTYPE_SERVER_SEED")) {
    std::uint32_t seed = 0;
    if (TryParseBounded(std::string_view(seed_value),
                        std::numeric_limits<std::uint32_t>::min(),
                        std::numeric_limits<std::uint32_t>::max(), seed)) {
      config.seed = seed;
    }
  }

  if (const char* room_code = std::getenv("RTYPE_SERVER_ROOM_CODE")) {
    const std::string_view room_view(room_code);
    if (!room_view.empty() &&
        ValidateLength(room_view, protocol::kMaxRoomCodeLength)) {
      config.default_room_code.assign(room_view);
    }
  }
  if (const char* room_name = std::getenv("RTYPE_SERVER_ROOM_NAME")) {
    const std::string_view room_view(room_name);
    if (!room_view.empty() &&
        ValidateLength(room_view, protocol::kMaxRoomNameLength)) {
      config.default_room_name.assign(room_view);
    }
  }

  if (const char* log_level = std::getenv("RTYPE_SERVER_LOG_LEVEL")) {
    engine::util::LogLevel level;
    if (TryParseLogLevel(log_level, level)) {
      config.log_level = level;
    }
  }

  return config;
}

}  // namespace server
