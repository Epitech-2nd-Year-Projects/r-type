#include "logging.h"

#include <mutex>
#include <utility>

namespace client {

namespace {

void EnsureLoggerName(engine::util::Logger& logger) {
  static std::once_flag once;
  std::call_once(once, [&logger]() { logger.SetName("client"); });
}

template <typename... Args>
void LogTagged(engine::util::LogLevel level, std::string_view tag,
               Args&&... args) {
  ClientLogger().Log(level, "[", tag, "] ", std::forward<Args>(args)...);
}

}  // namespace

engine::util::Logger& ClientLogger() {
  auto& logger = engine::util::Logger::Default();
  EnsureLoggerName(logger);
  return logger;
}

void ConfigureClientLogging(engine::util::LogLevel level) {
  ClientLogger().SetLevel(level);
}

void LogLifecycle(engine::util::LogLevel level, std::string_view message) {
  LogTagged(level, "lifecycle", message);
}

void LogNetwork(engine::util::LogLevel level, std::string_view message) {
  LogTagged(level, "network", message);
}

void LogLobby(engine::util::LogLevel level, std::string_view message) {
  LogTagged(level, "lobby", message);
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

}  // namespace client
