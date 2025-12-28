#ifndef CLIENT_LOGGING_H_
#define CLIENT_LOGGING_H_

#include <cstdint>
#include <string_view>

#include "engine/util/logging.h"

namespace client {

/**
 * @brief Access the client logger configured for this process
 */
engine::util::Logger& ClientLogger();

/**
 * @brief Configure logging based on runtime preferences
 */
void ConfigureClientLogging(engine::util::LogLevel level);

/**
 * @brief Log lifecycle information for major client stages
 */
void LogLifecycle(engine::util::LogLevel level, std::string_view message);

/**
 * @brief Log network subsystem messages
 */
void LogNetwork(engine::util::LogLevel level, std::string_view message);

/**
 * @brief Log lobby subsystem messages
 */
void LogLobby(engine::util::LogLevel level, std::string_view message);

/**
 * @brief Log connection status for the configured endpoint
 */
void LogConnectionStatus(engine::util::LogLevel level, std::string_view host,
                         std::uint16_t port, std::string_view detail);

/**
 * @brief Report packet level errors through the logger
 */
void LogPacketError(std::string_view stage, std::string_view detail);

}  // namespace client

#endif  // CLIENT_LOGGING_H_
