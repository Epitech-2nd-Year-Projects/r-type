#include "client_config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "constants/client_constants.h"
#include "constants/config_keys.h"
#include "engine/util/logging.h"
#include "protocol/join.h"

namespace client {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();
constexpr std::size_t kMaxHostLength = 255;
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

std::string TrimCopy(std::string_view text) {
  std::size_t start = 0;
  std::size_t end = text.size();
  while (start < end &&
         std::isspace(static_cast<unsigned char>(text[start])) != 0) {
    ++start;
  }
  while (end > start &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

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

bool ParsePortWithMessage(std::string_view value, std::uint16_t& out_port,
                          std::string& error) {
  unsigned int parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (ec != std::errc() || ptr != value.data() + value.size()) {
    error = "Invalid port";
    return false;
  }
  if (parsed < kMinPort || parsed > kMaxPort) {
    error = "Port must be between 1 and 65535";
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

bool TryParseHost(std::string_view value, std::string& out_host) {
  const std::string trimmed = TrimCopy(value);
  if (trimmed.empty()) {
    return false;
  }
  if (trimmed.size() > kMaxHostLength) {
    return false;
  }
  const bool has_space =
      std::any_of(trimmed.begin(), trimmed.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
      });
  if (has_space) {
    return false;
  }
  out_host = trimmed;
  return true;
}

bool TryParsePlayerName(std::string_view value, std::string& out_name) {
  const std::string trimmed = TrimCopy(value);
  if (trimmed.empty()) {
    return false;
  }
  if (!ValidateLength(trimmed, protocol::kMaxPlayerNameLength)) {
    return false;
  }
  out_name = trimmed;
  return true;
}

bool TryParseRoomCode(std::string_view value, std::string& out_room) {
  const std::string trimmed = TrimCopy(value);
  if (trimmed.empty()) {
    return false;
  }
  if (!ValidateLength(trimmed, protocol::kMaxRoomCodeLength)) {
    return false;
  }
  out_room = trimmed;
  return true;
}

bool TryParsePortValue(const nlohmann::json& value,
                       std::uint16_t& out_port) {
  if (value.is_number_integer() || value.is_number_unsigned()) {
    const auto parsed = value.get<long long>();
    if (parsed < kMinPort || parsed > kMaxPort) {
      return false;
    }
    out_port = static_cast<std::uint16_t>(parsed);
    return true;
  }
  if (value.is_string()) {
    return TryParsePort(value.get<std::string>(), out_port);
  }
  return false;
}

template <typename T>
bool TryParseRangeValue(const nlohmann::json& value, T& out_value, T min_value,
                        T max_value) {
  if (value.is_number_integer() || value.is_number_unsigned()) {
    return TryParseRange(std::to_string(value.get<long long>()), out_value,
                         min_value, max_value);
  }
  if (value.is_string()) {
    return TryParseRange(value.get<std::string>(), out_value, min_value,
                         max_value);
  }
  return false;
}

bool TryParseBoolValue(const nlohmann::json& value, bool& out_value) {
  if (value.is_boolean()) {
    out_value = value.get<bool>();
    return true;
  }
  if (value.is_string()) {
    const auto normalized = Normalize(value.get<std::string>());
    if (normalized == "1" || normalized == "true" || normalized == "yes") {
      out_value = true;
      return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no") {
      out_value = false;
      return true;
    }
  }
  return false;
}

bool TryParseTimeout(std::string_view value, std::uint32_t& out_timeout) {
  return TryParseRange(value, out_timeout, kMinTimeoutMs, kMaxTimeoutMs);
}

bool TryParsePingInterval(std::string_view value, std::uint32_t& out_interval) {
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
  return TryParseRange(value, out_attempts, kMinJoinAttempts, kMaxJoinAttempts);
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

bool LoadClientConfigFromFile(const std::filesystem::path& path,
                              ClientConfig& config) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  nlohmann::json doc;
  try {
    file >> doc;
  } catch (const std::exception&) {
    return false;
  }

  if (!doc.is_object()) {
    return false;
  }

  const auto apply_text = [&](std::string_view key, auto parser,
                              std::string& target) {
    const auto it = doc.find(std::string(key));
    if (it == doc.end() || !it->is_string()) {
      return;
    }
    std::string parsed;
    if (parser(it->get<std::string>(), parsed)) {
      target = parsed;
    }
  };

  const auto apply_port = [&](std::string_view key, std::uint16_t& target) {
    const auto it = doc.find(std::string(key));
    if (it == doc.end()) {
      return;
    }
    std::uint16_t parsed{};
    if (TryParsePortValue(*it, parsed)) {
      target = parsed;
    }
  };

  apply_text(constants::config::kClientHost, TryParseHost, config.host);
  apply_port(constants::config::kClientPort, config.port);
  apply_text(constants::config::kClientPlayerName, TryParsePlayerName,
             config.player_name);
  apply_text(constants::config::kClientRoomCode, TryParseRoomCode,
             config.room_code);

  const auto apply_range = [&](std::string_view key, auto& target,
                               auto min_value, auto max_value) {
    const auto it = doc.find(std::string(key));
    if (it == doc.end()) {
      return;
    }
    using TargetType = std::remove_reference_t<decltype(target)>;
    TargetType parsed{};
    if (TryParseRangeValue(*it, parsed, static_cast<TargetType>(min_value),
                           static_cast<TargetType>(max_value))) {
      target = parsed;
    }
  };

  apply_range(constants::config::kClientTimeoutMs, config.timeout_ms,
              kMinTimeoutMs, kMaxTimeoutMs);
  apply_range(constants::config::kClientPingIntervalMs, config.ping_interval_ms,
              kMinPingIntervalMs, kMaxPingIntervalMs);
  apply_range(constants::config::kClientQueueSize, config.network_queue_size,
              kMinQueueSize, kMaxQueueSize);
  apply_range(constants::config::kClientJoinRetryMs, config.join_retry_delay_ms,
              kMinJoinRetryDelayMs, kMaxJoinRetryDelayMs);
  apply_range(constants::config::kClientJoinMaxAttempts,
              config.join_max_attempts, kMinJoinAttempts, kMaxJoinAttempts);
  apply_range(constants::config::kClientLobbyRetryMs,
              config.lobby_retry_delay_ms, kMinLobbyRetryDelayMs,
              kMaxLobbyRetryDelayMs);
  apply_range(constants::config::kClientLobbyMaxAttempts,
              config.lobby_max_attempts, kMinLobbyAttempts, kMaxLobbyAttempts);

  if (const auto it = doc.find(std::string(constants::config::kClientDebug));
      it != doc.end()) {
    bool parsed = config.debug;
    if (TryParseBoolValue(*it, parsed)) {
      config.debug = parsed;
    }
  }

  if (const auto it =
          doc.find(std::string(constants::config::kClientLogLevel));
      it != doc.end() && it->is_string()) {
    engine::util::LogLevel parsed{};
    if (TryParseLogLevel(it->get<std::string>(), parsed)) {
      config.log_level = parsed;
    }
  }

  return true;
}

bool SaveClientConfigToFile(const std::filesystem::path& path,
                            const ClientConfig& config) {
  const auto parent = path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
  }

  nlohmann::json doc = nlohmann::json::object();
  doc[std::string(constants::config::kClientHost)] = config.host;
  doc[std::string(constants::config::kClientPort)] = config.port;
  doc[std::string(constants::config::kClientPlayerName)] = config.player_name;
  doc[std::string(constants::config::kClientRoomCode)] = config.room_code;
  doc[std::string(constants::config::kClientDebug)] = config.debug;
  doc[std::string(constants::config::kClientLogLevel)] =
      std::string(engine::util::ToString(config.log_level));
  doc[std::string(constants::config::kClientTimeoutMs)] = config.timeout_ms;
  doc[std::string(constants::config::kClientPingIntervalMs)] =
      config.ping_interval_ms;
  doc[std::string(constants::config::kClientQueueSize)] =
      config.network_queue_size;
  doc[std::string(constants::config::kClientJoinRetryMs)] =
      config.join_retry_delay_ms;
  doc[std::string(constants::config::kClientJoinMaxAttempts)] =
      config.join_max_attempts;
  doc[std::string(constants::config::kClientLobbyRetryMs)] =
      config.lobby_retry_delay_ms;
  doc[std::string(constants::config::kClientLobbyMaxAttempts)] =
      config.lobby_max_attempts;

  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }
  file << doc.dump(2);
  return true;
}

}  // namespace

ClientConfig LoadClientConfig() {
  ClientConfig config;
  LoadClientConfigFromFile(
      std::filesystem::path(constants::client::kClientConfigPath), config);

  const auto apply_uint = [&](const char* env_name, auto parser, auto& target) {
    const char* value = std::getenv(env_name);
    if (!value) return;
    typename std::remove_reference_t<decltype(target)> parsed{};
    if (parser(std::string_view(value), parsed)) {
      target = parsed;
    }
  };

  if (const char* host = std::getenv("RTYPE_CLIENT_HOST")) {
    std::string parsed;
    if (TryParseHost(host, parsed)) {
      config.host = parsed;
    }
  }

  apply_uint("RTYPE_CLIENT_PORT", TryParsePort, config.port);

  if (const char* name = std::getenv("RTYPE_CLIENT_NAME")) {
    std::string parsed;
    if (TryParsePlayerName(name, parsed)) {
      config.player_name = parsed;
    }
  }

  if (const char* room = std::getenv("RTYPE_CLIENT_ROOM")) {
    std::string parsed;
    if (TryParseRoomCode(room, parsed)) {
      config.room_code = parsed;
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

bool SaveClientConfig(const ClientConfig& config) {
  return SaveClientConfigToFile(
      std::filesystem::path(constants::client::kClientConfigPath), config);
}

ConnectionValidationResult ValidateConnectionFields(
    std::string_view host, std::string_view port_text,
    std::string_view player_name) {
  ConnectionValidationResult result{};
  const ClientConfig defaults{};

  const std::string trimmed_host = TrimCopy(host);
  if (trimmed_host.empty()) {
    result.host = defaults.host;
  } else {
    if (trimmed_host.size() > kMaxHostLength) {
      result.message = "Host must be 255 characters or fewer";
      return result;
    }
    const bool has_space =
        std::any_of(trimmed_host.begin(), trimmed_host.end(),
                    [](unsigned char c) { return std::isspace(c) != 0; });
    if (has_space) {
      result.message = "Host must not contain spaces";
      return result;
    }
    result.host = trimmed_host;
  }

  const std::string trimmed_port = TrimCopy(port_text);
  if (trimmed_port.empty()) {
    result.port = defaults.port;
  } else {
    std::string error;
    if (!ParsePortWithMessage(trimmed_port, result.port, error)) {
      result.message = error;
      return result;
    }
  }

  const std::string trimmed_name = TrimCopy(player_name);
  if (trimmed_name.empty()) {
    result.player_name = defaults.player_name;
  } else {
    if (!ValidateLength(trimmed_name, protocol::kMaxPlayerNameLength)) {
      result.message = "Name must be " +
                       std::to_string(protocol::kMaxPlayerNameLength) +
                       " characters or fewer";
      return result;
    }
    result.player_name = trimmed_name;
  }

  result.valid = true;
  return result;
}

}  // namespace client
