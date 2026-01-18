#include "logging.h"

#include <mutex>
#include <utility>

namespace rift::client {

namespace {

void EnsureLoggerName(engine::util::Logger& logger) {
  static std::once_flag once;
  std::call_once(once, [&logger]() { logger.SetName("rift_client"); });
}

template <typename... Args>
void LogTagged(engine::util::LogLevel level, std::string_view tag,
               Args&&... args) {
  RiftClientLogger().Log(level, "[", tag, "] ", std::forward<Args>(args)...);
}

}  // namespace

engine::util::Logger& RiftClientLogger() {
  auto& logger = engine::util::Logger::Default();
  EnsureLoggerName(logger);
  return logger;
}

void ConfigureRiftClientLogging(engine::util::LogLevel level) {
  RiftClientLogger().SetLevel(level);
}

void LogLifecycle(engine::util::LogLevel level, std::string_view message) {
  LogTagged(level, "lifecycle", message);
}

void LogNetwork(engine::util::LogLevel level, std::string_view message) {
  LogTagged(level, "network", message);
}

void LogConnectionStatus(engine::util::LogLevel level, std::string_view host,
                         std::uint16_t port, std::string_view detail) {
  LogTagged(level, "network", "Connection ", detail, " [", host, ":", port,
            "]");
}

void LogPacketError(std::string_view stage, std::string_view detail) {
  LogTagged(engine::util::LogLevel::kError, "network", "Packet error during ",
            stage, ": ", detail);
}

}  // namespace rift::client
