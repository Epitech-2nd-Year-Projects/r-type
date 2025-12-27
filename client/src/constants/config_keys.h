/**
 * @file config_keys
 * @brief Configuration key names for the client runtime
 */

#ifndef CLIENT_CONSTANTS_CONFIG_KEYS_H_
#define CLIENT_CONSTANTS_CONFIG_KEYS_H_

#include <string_view>

namespace client::constants::config {

inline constexpr std::string_view kClientHost = "client.host";
inline constexpr std::string_view kClientPort = "client.port";
inline constexpr std::string_view kClientDebug = "client.debug";
inline constexpr std::string_view kClientLogLevel = "client.log_level";
inline constexpr std::string_view kClientPlayerName = "client.player_name";
inline constexpr std::string_view kClientRoomCode = "client.room_code";
inline constexpr std::string_view kClientTimeoutMs = "client.timeout_ms";

}  // namespace client::constants::config

#endif  // CLIENT_CONSTANTS_CONFIG_KEYS_H_
