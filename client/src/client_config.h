#ifndef CLIENT_CLIENT_CONFIG_H_
#define CLIENT_CLIENT_CONFIG_H_

#include <cstdint>
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
  engine::util::LogLevel log_level{engine::util::LogLevel::kDebug};
};

/**
 * @brief Load client configuration using environment overrides.
 *
 * Environment overrides:
 *  - RTYPE_CLIENT_HOST
 *  - RTYPE_CLIENT_PORT
 *  - RTYPE_CLIENT_NAME
 *  - RTYPE_CLIENT_ROOM
 *  - RTYPE_CLIENT_DEBUG
 *  - RTYPE_CLIENT_TIMEOUT_MS
 *  - RTYPE_CLIENT_LOG_LEVEL
 */
ClientConfig LoadClientConfig();

}  // namespace client

#endif  // CLIENT_CLIENT_CONFIG_H_
