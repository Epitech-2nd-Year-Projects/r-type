#ifndef CLIENT_CLIENT_CONFIG_H_
#define CLIENT_CLIENT_CONFIG_H_

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "engine/util/logging.h"

namespace client {

/**
 * @brief User provided configuration for the client runtime
 */
struct ClientConfig {
  std::string host{"127.0.0.1"};
  std::uint16_t port{4242};
  std::string player_name{"Pilot"};
  std::string room_code{"default"};
  bool debug{false};
  std::uint32_t timeout_ms{7'000};
  engine::util::LogLevel log_level{engine::util::LogLevel::kInfo};
};

/**
 * @brief Outcome of parsing command line arguments into a ClientConfig
 */
struct ClientConfigParseResult {
  ClientConfig config{};
  bool ok{true};
  std::string error;
};

/**
 * @brief Parse CLI flags into a ClientConfig instance
 */
ClientConfigParseResult ParseClientConfig(
    std::span<const std::string_view> args);

}  // namespace client

#endif  // CLIENT_CLIENT_CONFIG_H_
