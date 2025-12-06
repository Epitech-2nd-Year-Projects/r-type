#include "client_config.h"

#include <charconv>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace client {

namespace {

constexpr std::uint16_t kMinPort = 1;
constexpr std::uint16_t kMaxPort = std::numeric_limits<std::uint16_t>::max();

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

    return MakeError(config, "Unknown argument: " + std::string(arg));
  }

  return ClientConfigParseResult{config, true, {}};
}

}  // namespace client
