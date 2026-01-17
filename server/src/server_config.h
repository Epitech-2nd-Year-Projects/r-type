#ifndef SERVER_SERVER_CONFIG_H_
#define SERVER_SERVER_CONFIG_H_

#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "engine/util/logging.h"

namespace server {

/**
 * @brief Runtime configuration for the dedicated server process.
 *
 * Environment overrides:
 *  - RTYPE_SERVER_PORT
 *  - RTYPE_SERVER_MAX_PLAYERS
 *  - RTYPE_SERVER_TICK_RATE
 *  - RTYPE_SERVER_TIMEOUT_MS
 *  - RTYPE_SERVER_ROOM_IDLE_TIMEOUT_MS
 *  - RTYPE_SERVER_ROOM_CODE
 *  - RTYPE_SERVER_SEED
 *  - RTYPE_SERVER_LOG_LEVEL
 */
struct ServerConfig {
  std::uint16_t port{4242};                                       ///< UDP port to bind the server socket (default: 4242).
  std::uint16_t max_players{4};                                   ///< Default capacity for newly created rooms.
  std::uint16_t tick_rate{30};                                    ///< Server simulation ticks per second (default: 30 Hz).
  std::uint32_t peer_timeout_ms{15'000};                          ///< Milliseconds of inactivity before a peer is considered disconnected (default: 15000 ms).
  std::uint32_t room_idle_timeout_ms{30'000};                     ///< Milliseconds of inactivity before an empty room is reclaimed (default: 30000 ms).
  std::string default_room_code{""};                              ///< Optional default room code (unused when empty).
  std::string default_room_name{"Default"};                       ///< Display name for the default room.
  std::uint32_t seed{std::random_device{}()};                     ///< Random seed for deterministic simulation (default: random).
  engine::util::LogLevel log_level{engine::util::LogLevel::kInfo}; ///< Logging verbosity level (default: Info).
};

/**
 * @brief Loads configuration using environment overrides with safe defaults.
 */
ServerConfig LoadServerConfig();

}  // namespace server

#endif  // SERVER_SERVER_CONFIG_H_
