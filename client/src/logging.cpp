#include "logging.h"

#include <mutex>
#include <utility>

namespace client {

namespace {

void EnsureLoggerName(engine::util::Logger& logger) {
  static std::once_flag once;
  std::call_once(once, [&logger]() { logger.SetName("client"); });
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
  ClientLogger().Log(level, message);
}

void LogConnectionStatus(engine::util::LogLevel level, std::string_view host,
                         std::uint16_t port, std::string_view detail) {
  ClientLogger().Log(level, "Connection ", detail, " [", host, ":", port, "]");
}

void LogPacketError(std::string_view stage, std::string_view detail) {
  ClientLogger().Log(engine::util::LogLevel::kError,
                     "Packet error during ", stage, ": ", detail);
}

}  // namespace client
