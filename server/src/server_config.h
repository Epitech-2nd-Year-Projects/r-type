#ifndef SERVER_SERVER_CONFIG_H_
#define SERVER_SERVER_CONFIG_H_

#include <cstdint>
#include <random>
#include <string>

#include "engine/util/logging.h"

namespace server {

/**
 * @brief Runtime configuration for the dedicated server process
 */
struct ServerConfig {
  std::uint16_t port{4242};
  std::uint16_t max_players{4};
  std::uint16_t tick_rate{60};
  std::string room_name{"default"};
  std::uint32_t seed{std::random_device{}()};
  engine::util::LogLevel log_level{engine::util::LogLevel::kInfo};
};

/**
 * @brief Outcome of parsing CLI arguments into a ServerConfig instance
 */
struct ServerConfigParseResult {
  ServerConfig config{};
  bool ok{true};
  std::string error;
};

/**
 * @brief Parse CLI flags into a ServerConfig instance
 */
ServerConfigParseResult ParseServerConfig(int argc, char** argv);

}  // namespace server

#endif  // SERVER_SERVER_CONFIG_H_
