#ifndef SERVER_SERVER_CONFIG_H_
#define SERVER_SERVER_CONFIG_H_

#include <cstdint>
#include <random>
#include <string>
#include <span>
#include <string_view>

#include "engine/util/logging.h"

namespace server {

/**
 * @brief Runtime configuration for the dedicated server process.
 * 
 * Contains all configurable parameters for server operation including
 * network settings, gameplay limits, simulation parameters, and diagnostics.
 */
struct ServerConfig {
  std::uint16_t port{4242};                                       ///< UDP port to bind the server socket (default: 4242).
  std::uint16_t max_players{4};                                   ///< Maximum number of concurrent players allowed (default: 4).
  std::uint16_t tick_rate{60};                                    ///< Server simulation ticks per second (default: 60 Hz).
  std::uint32_t peer_timeout_ms{15'000};                          ///< Milliseconds of inactivity before a peer is considered disconnected (default: 15000 ms).
  std::string room_code{};                                        ///< Optional room code for matchmaking/filtering.
  std::uint32_t seed{std::random_device{}()};                    ///< Random seed for deterministic simulation (default: random).
  engine::util::LogLevel log_level{engine::util::LogLevel::kInfo}; ///< Logging verbosity level (default: Info).
};

/**
 * @brief Result of parsing command-line arguments into a ServerConfig.
 * 
 * Contains the parsed configuration and status information indicating
 * whether parsing succeeded or failed.
 */
struct ServerConfigParseResult {
  ServerConfig config{};  ///< Parsed server configuration (valid if ok is true).
  bool ok{true};          ///< true if parsing succeeded, false on error.
  std::string error;      ///< Error message describing parsing failure (empty if ok is true).
};

/**
 * @brief Parses command-line arguments into a ServerConfig instance.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return ServerConfigParseResult containing the parsed config and status.
 * 
 * Supported flags:
 *   --port <num>       : UDP port to bind (default: 4242)
 *   --max-players <num>: Maximum concurrent players (default: 4)
 *   --tick-rate <num>  : Server tick rate in Hz (default: 60)
 *   --room-code <str>  : Optional room code
 *   --seed <num>       : Random seed for deterministic behavior
 *   --log-level <str>  : Log level (trace, debug, info, warn, error)
 * 
 * If parsing fails, the result will have ok=false and error will contain
 * a descriptive message.
 */
ServerConfigParseResult ParseServerConfig(
    std::span<const std::string_view> args);

}  // namespace server

#endif  // SERVER_SERVER_CONFIG_H_
