#include "client_config.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include "engine/util/logging.h"

namespace client {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();

std::string Normalize(std::string_view value) {
  std::string normalized(value.size(), '\0');
  std::transform(value.begin(), value.end(), normalized.begin(),
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

bool TryParsePort(std::string_view value, std::uint16_t* out_port) {
  unsigned int parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto [ptr, ec] = std::from_chars(begin, end, parsed);
  if (ec != std::errc() || ptr != end) {
    return false;
  }
  if (parsed < kMinPort || parsed > kMaxPort) {
    return false;
  }
  *out_port = static_cast<std::uint16_t>(parsed);
  return true;
}

bool TryParseLogLevel(std::string_view value,
                      engine::util::LogLevel* out_level) {
  const auto normalized = Normalize(value);
  const auto level =
      engine::util::ParseLogLevel(normalized, engine::util::LogLevel::kInfo);

  switch (level) {
    case engine::util::LogLevel::kTrace:
      if (normalized == "trace") break;
      return false;
    case engine::util::LogLevel::kDebug:
      if (normalized == "debug") break;
      return false;
    case engine::util::LogLevel::kInfo:
      if (normalized == "info") break;
      return false;
    case engine::util::LogLevel::kWarn:
      if (normalized == "warn" || normalized == "warning") break;
      return false;
    case engine::util::LogLevel::kError:
      if (normalized == "error") break;
      return false;
    case engine::util::LogLevel::kCritical:
      if (normalized == "critical" || normalized == "fatal") break;
      return false;
    case engine::util::LogLevel::kOff:
      if (normalized == "off" || normalized == "none") break;
      return false;
    default:
      return false;
  }

  *out_level = level;
  return true;
}

}  // namespace

ClientConfigParseResult ParseClientConfig(int argc, char** argv) {
  ClientConfig config;
  std::span args(argv, static_cast<std::size_t>(argc));

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
      if (!TryParsePort(args[i + 1], &port)) {
        return MakeError(config,
                         "Invalid port supplied (must be between 1 and 65535)");
      }
      config.port = port;
      ++i;
      continue;
    }

    if (arg == "--debug") {
      config.debug = true;
      continue;
    }

    if (arg == "--log-level") {
      if (i + 1 >= args.size()) {
        return MakeError(config, "Missing value for --log-level");
      }
      engine::util::LogLevel level;
      if (!TryParseLogLevel(args[i + 1], &level)) {
        return MakeError(config,
                         "Invalid log level (trace, debug, info, warn, error, critical, off)");
      }
      config.log_level = level;
      ++i;
      continue;
    }

    return MakeError(config, "Unknown argument: " + std::string(arg));
  }

  if (config.debug &&
      config.log_level > engine::util::LogLevel::kDebug) {
    config.log_level = engine::util::LogLevel::kDebug;
  }

  return ClientConfigParseResult{config, true, {}};
}

}  // namespace client
