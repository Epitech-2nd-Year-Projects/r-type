#ifndef RIFT_CLIENT_LOGGING_H_
#define RIFT_CLIENT_LOGGING_H_

#include <cstdint>
#include <string_view>

#include "engine/util/logging.h"

namespace rift::client {

engine::util::Logger& RiftClientLogger();

void ConfigureRiftClientLogging(engine::util::LogLevel level);

void LogLifecycle(engine::util::LogLevel level, std::string_view message);

void LogNetwork(engine::util::LogLevel level, std::string_view message);

void LogConnectionStatus(engine::util::LogLevel level, std::string_view host,
                         std::uint16_t port, std::string_view detail);

void LogPacketError(std::string_view stage, std::string_view detail);

}  // namespace rift::client

#endif  // RIFT_CLIENT_LOGGING_H_
