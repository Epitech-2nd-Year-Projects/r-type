#include "client_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#include "engine/util/logging.h"
#include "protocol/join.h"

namespace client {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();
constexpr std::uint32_t kMinTimeoutMs = 3'000;
constexpr std::uint32_t kMaxTimeoutMs = 30'000;
constexpr std::uint32_t kMinPingIntervalMs = 100;
constexpr std::uint32_t kMaxPingIntervalMs = 10'000;
constexpr std::size_t kMinQueueSize = 32;
constexpr std::size_t kMaxQueueSize = 4'096;
constexpr std::uint32_t kMinJoinRetryDelayMs = 100;
constexpr std::uint32_t kMaxJoinRetryDelayMs = 5'000;
constexpr int kMinJoinAttempts = 1;
constexpr int kMaxJoinAttempts = 20;
constexpr std::uint32_t kMinLobbyRetryDelayMs = 100;
constexpr std::uint32_t kMaxLobbyRetryDelayMs = 5'000;
constexpr int kMinLobbyAttempts = 1;
constexpr int kMaxLobbyAttempts = 20;

std::string Normalize(std::string_view value) {
  std::string normalized(value.size(), '\0');
  std::transform(
      value.begin(), value.end(), normalized.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return normalized;
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

template <typename T>
bool TryParseRange(std::string_view value, T& out_value, T min_value,
                   T max_value) {
  T parsed{};
  const auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc() || ptr != value.data() + value.size()) {
    return false;
  }
  if (parsed < min_value || parsed > max_value) {
    return false;
  }
  out_value = parsed;
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
  return TryParseRange(value, out_timeout, kMinTimeoutMs, kMaxTimeoutMs);
}

bool TryParsePingInterval(std::string_view value,
                          std::uint32_t& out_interval) {
  return TryParseRange(value, out_interval, kMinPingIntervalMs,
                       kMaxPingIntervalMs);
}

bool TryParseQueueSize(std::string_view value, std::size_t& out_queue_size) {
  return TryParseRange(value, out_queue_size, kMinQueueSize, kMaxQueueSize);
}

bool TryParseJoinRetryDelay(std::string_view value,
                            std::uint32_t& out_delay_ms) {
  return TryParseRange(value, out_delay_ms, kMinJoinRetryDelayMs,
                       kMaxJoinRetryDelayMs);
}

bool TryParseJoinAttempts(std::string_view value, int& out_attempts) {
  return TryParseRange(value, out_attempts, kMinJoinAttempts,
                       kMaxJoinAttempts);
}

bool TryParseLobbyRetryDelay(std::string_view value,
                             std::uint32_t& out_delay_ms) {
  return TryParseRange(value, out_delay_ms, kMinLobbyRetryDelayMs,
                       kMaxLobbyRetryDelayMs);
}

bool TryParseLobbyAttempts(std::string_view value, int& out_attempts) {
  return TryParseRange(value, out_attempts, kMinLobbyAttempts,
                       kMaxLobbyAttempts);
}

}  // namespace

ClientConfig LoadClientConfig() {
  ClientConfig config;

  const auto apply_uint = [&](const char* env_name, auto parser, auto& target) {
    const char* value = std::getenv(env_name);
    if (!value) return;
    typename std::remove_reference_t<decltype(target)> parsed{};
    if (parser(std::string_view(value), parsed)) {
      target = parsed;
    }
  };

  if (const char* host = std::getenv("RTYPE_CLIENT_HOST")) {
    if (std::string_view(host).size() > 0) {
      config.host = host;
    }
  }

  apply_uint("RTYPE_CLIENT_PORT", TryParsePort, config.port);

  if (const char* name = std::getenv("RTYPE_CLIENT_NAME")) {
    const std::string_view name_view(name);
    if (!name_view.empty() &&
        ValidateLength(name_view, protocol::kMaxPlayerNameLength)) {
      config.player_name.assign(name_view);
    }
  }

  if (const char* room = std::getenv("RTYPE_CLIENT_ROOM")) {
    const std::string_view room_view(room);
    if (!room_view.empty() &&
        ValidateLength(room_view, protocol::kMaxRoomCodeLength)) {
      config.room_code.assign(room_view);
    }
  }

  apply_uint("RTYPE_CLIENT_TIMEOUT_MS", TryParseTimeout, config.timeout_ms);
  apply_uint("RTYPE_CLIENT_PING_INTERVAL_MS", TryParsePingInterval,
             config.ping_interval_ms);
  apply_uint("RTYPE_CLIENT_QUEUE_SIZE", TryParseQueueSize,
             config.network_queue_size);
  apply_uint("RTYPE_CLIENT_JOIN_RETRY_MS", TryParseJoinRetryDelay,
             config.join_retry_delay_ms);
  apply_uint("RTYPE_CLIENT_JOIN_MAX_ATTEMPTS", TryParseJoinAttempts,
             config.join_max_attempts);
  apply_uint("RTYPE_CLIENT_LOBBY_RETRY_MS", TryParseLobbyRetryDelay,
             config.lobby_retry_delay_ms);
  apply_uint("RTYPE_CLIENT_LOBBY_MAX_ATTEMPTS", TryParseLobbyAttempts,
             config.lobby_max_attempts);

  if (const char* debug = std::getenv("RTYPE_CLIENT_DEBUG")) {
    const auto normalized = Normalize(debug);
    if (normalized == "1" || normalized == "true" || normalized == "yes") {
      config.debug = true;
    }
  }

  if (const char* log_level = std::getenv("RTYPE_CLIENT_LOG_LEVEL")) {
    engine::util::LogLevel level;
    if (TryParseLogLevel(log_level, level)) {
      config.log_level = level;
    }
  }

  if (config.debug && config.log_level > engine::util::LogLevel::kDebug) {
    config.log_level = engine::util::LogLevel::kDebug;
  }

  return config;
}

}  // namespace client
