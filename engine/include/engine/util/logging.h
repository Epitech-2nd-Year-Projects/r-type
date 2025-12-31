#ifndef ENGINE_ENGINE_UTIL_LOGGING_H_
#define ENGINE_ENGINE_UTIL_LOGGING_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace engine::util {

/**
 * @brief Severity levels understood by the logging system.
 */
enum class LogLevel : std::uint8_t {
  kTrace = 0,
  kDebug,
  kInfo,
  kWarn,
  kError,
  kCritical,
  kOff
};

/**
 * @brief Convert a LogLevel to its textual representation.
 */
std::string_view ToString(LogLevel level) noexcept;

/**
 * @brief Parse a textual representation of a log level.
 */
LogLevel ParseLogLevel(std::string_view level_name,
                       LogLevel fallback = LogLevel::kInfo) noexcept;

/**
 * @brief Single log event routed to sinks.
 */
struct LogMessage {
  LogLevel level = LogLevel::kInfo;
  std::string text;
  std::string logger;
  std::chrono::system_clock::time_point timestamp;
  std::thread::id thread_id;
};

/**
 * @brief Base class implemented by concrete log sinks (console, file, ...).
 */
class LogSink {
 public:
  virtual ~LogSink() = default;

  /**
   * @brief Consume a log message.
   */
  virtual void Write(const LogMessage& message) = 0;

  /**
   * @brief Flush buffered data when supported by the sink.
   */
  virtual void Flush() {}
};

/**
 * @brief Writes log messages to std::cout/std::cerr.
 */
class ConsoleSink : public LogSink {
 public:
  explicit ConsoleSink(bool colorize = true);
  ConsoleSink(std::ostream& out, std::ostream& err, bool colorize = true);

  void Write(const LogMessage& message) override;
  void Flush() override;

 private:
  std::ostream& out_;
  std::ostream& err_;
  bool colorize_;
  std::mutex mutex_;
};

/**
 * @brief Persists log messages to a file on disk.
 */
class FileSink : public LogSink {
 public:
  explicit FileSink(std::filesystem::path path, bool append = true);

  void Write(const LogMessage& message) override;
  void Flush() override;

  [[nodiscard]] bool IsOpen() const;
  void Reopen(bool append = true);

 private:
  void Open(bool append);

  std::filesystem::path path_;
  mutable std::mutex mutex_;
  std::ofstream stream_;
  bool append_mode_ = true;
};

/**
 * @brief Thread-safe logger dispatching messages to registered sinks.
 */
class Logger {
 public:
  Logger();
  explicit Logger(std::string name);

  void SetLevel(LogLevel level) noexcept;
  [[nodiscard]] LogLevel Level() const noexcept;

  void AddSink(std::shared_ptr<LogSink> sink);
  void RemoveSink(const std::shared_ptr<LogSink>& sink);
  void ClearSinks();
  void SetName(std::string name);

  template <typename... Args>
  void Log(LogLevel level, Args&&... args) {
    const auto current_level = level_.load(std::memory_order_relaxed);
    if (current_level == LogLevel::kOff || level < current_level) {
      return;
    }

    LogMessage message;
    message.level = level;
    {
      std::lock_guard lock(name_mutex_);
      message.logger = name_;
    }
    message.timestamp = std::chrono::system_clock::now();
    message.thread_id = std::this_thread::get_id();
    message.text = BuildMessage(std::forward<Args>(args)...);
    Dispatch(std::move(message));
  }

  template <typename... Args>
  void Trace(Args&&... args) {
    Log(LogLevel::kTrace, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Debug(Args&&... args) {
    Log(LogLevel::kDebug, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Info(Args&&... args) {
    Log(LogLevel::kInfo, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Warn(Args&&... args) {
    Log(LogLevel::kWarn, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Error(Args&&... args) {
    Log(LogLevel::kError, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Critical(Args&&... args) {
    Log(LogLevel::kCritical, std::forward<Args>(args)...);
  }

  /**
   * @brief Retrieve the process-wide default logger.
   */
  static Logger& Default();

  /**
   * @brief Configure the default logger at runtime.
   */
  static void ConfigureDefaultLevel(LogLevel level) noexcept;
  static void AddDefaultSink(const std::shared_ptr<LogSink>& sink);
  static void ClearDefaultSinks();

 private:
  template <typename... Args>
  static std::string BuildMessage(Args&&... args) {
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args));
    return stream.str();
  }

  void Dispatch(LogMessage message);

  std::string name_ = "engine";
  mutable std::mutex name_mutex_;
  std::vector<std::shared_ptr<LogSink>> sinks_;
  mutable std::mutex sinks_mutex_;
  std::atomic<LogLevel> level_{LogLevel::kInfo};
};

#define ENGINE_LOG_TRACE(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kTrace, \
                                        __VA_ARGS__)

#define ENGINE_LOG_DEBUG(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kDebug, \
                                        __VA_ARGS__)

#define ENGINE_LOG_INFO(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kInfo, \
                                        __VA_ARGS__)

#define ENGINE_LOG_WARN(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kWarn, \
                                        __VA_ARGS__)

#define ENGINE_LOG_ERROR(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kError, \
                                        __VA_ARGS__)

#define ENGINE_LOG_CRITICAL(...)                                             \
  ::engine::util::Logger::Default().Log(::engine::util::LogLevel::kCritical, \
                                        __VA_ARGS__)

}  // namespace engine::util

#endif /* !ENGINE_ENGINE_UTIL_LOGGING_H_ */
