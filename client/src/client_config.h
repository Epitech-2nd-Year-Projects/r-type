#ifndef CLIENT_CLIENT_CONFIG_H_
#define CLIENT_CLIENT_CONFIG_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "constants/client_constants.h"
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
  std::uint32_t ping_interval_ms{1'000};
  std::size_t network_queue_size{256};
  std::uint32_t join_retry_delay_ms{500};
  int join_max_attempts{5};
  std::uint32_t lobby_retry_delay_ms{400};
  int lobby_max_attempts{4};
  std::uint32_t room_list_refresh_ms{5'000};
  std::uint32_t interpolation_delay_ms{100};
  std::uint32_t max_extrapolation_ms{150};
  float master_volume{1.0f};
  float music_volume{1.0f};
  float sfx_volume{1.0f};
  int resolution_width{constants::client::kBaseResolution.x};
  int resolution_height{constants::client::kBaseResolution.y};
  bool fullscreen{false};
  bool vsync{constants::client::kWindowVsync};
  int target_fps{constants::client::kTargetFps};
  engine::util::LogLevel log_level{engine::util::LogLevel::kDebug};
};

/**
 * @struct ConnectionValidationResult
 * @brief Result for connection input validation
 */
struct ConnectionValidationResult {
  bool valid{false};
  std::string message{};
  std::string host{};
  std::uint16_t port{0};
  std::string player_name{};
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
 *  - RTYPE_CLIENT_PING_INTERVAL_MS
 *  - RTYPE_CLIENT_QUEUE_SIZE
 *  - RTYPE_CLIENT_JOIN_RETRY_MS
 *  - RTYPE_CLIENT_JOIN_MAX_ATTEMPTS
 *  - RTYPE_CLIENT_LOBBY_RETRY_MS
 *  - RTYPE_CLIENT_LOBBY_MAX_ATTEMPTS
 *  - RTYPE_CLIENT_ROOM_LIST_REFRESH_MS
 *  - RTYPE_CLIENT_INTERPOLATION_DELAY_MS
 *  - RTYPE_CLIENT_MAX_EXTRAPOLATION_MS
 *  - RTYPE_CLIENT_LOG_LEVEL
 */

ClientConfig LoadClientConfig();

/**
 * @brief Persist client configuration to disk
 */
bool SaveClientConfig(const ClientConfig& config);

/**
 * @brief Validate host port and name inputs
 * @param host Input host
 * @param port_text Input port text
 * @param player_name Input name
 */
ConnectionValidationResult ValidateConnectionFields(
    std::string_view host, std::string_view port_text,
    std::string_view player_name);

}  // namespace client

#endif  // CLIENT_CLIENT_CONFIG_H_
