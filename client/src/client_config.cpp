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
