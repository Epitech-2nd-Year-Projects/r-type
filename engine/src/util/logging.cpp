#include "engine/util/logging.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace engine::util {

namespace {

std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) {
  using namespace std::chrono;
  const auto time = system_clock::to_time_t(tp);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
  oss << '.' << std::setw(3) << std::setfill('0') << ms.count();
  return oss.str();
}

std::string ComposeLine(const LogMessage& message) {
  std::ostringstream oss;
  oss << '[' << FormatTimestamp(message.timestamp) << "] ";
  if (!message.logger.empty()) {
    oss << '[' << message.logger << "] ";
  }
  oss << '[' << ToString(message.level) << "] " << message.text;
  return oss.str();
}

std::string_view ColorForLevel(LogLevel level) {
  switch (level) {
    case LogLevel::kTrace:
    case LogLevel::kDebug:
      return "\033[36m";  // cyan
    case LogLevel::kInfo:
      return "\033[32m";  // green
    case LogLevel::kWarn:
      return "\033[33m";  // yellow
    case LogLevel::kError:
      return "\033[31m";  // red
    case LogLevel::kCritical:
      return "\033[35m";  // magenta
    case LogLevel::kOff:
    default:
      return "";
  }
}

}  // namespace

std::string_view ToString(LogLevel level) noexcept {
  switch (level) {
    case LogLevel::kTrace:
      return "trace";
    case LogLevel::kDebug:
      return "debug";
    case LogLevel::kInfo:
      return "info";
    case LogLevel::kWarn:
      return "warn";
    case LogLevel::kError:
      return "error";
    case LogLevel::kCritical:
      return "critical";
    case LogLevel::kOff:
      return "off";
    default:
      return "unknown";
  }
}

LogLevel ParseLogLevel(std::string_view level_name,
                       LogLevel fallback) noexcept {
  std::string normalized(level_name.size(), '\0');
  std::transform(
      level_name.begin(), level_name.end(), normalized.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (normalized == "trace") return LogLevel::kTrace;
  if (normalized == "debug") return LogLevel::kDebug;
  if (normalized == "info") return LogLevel::kInfo;
  if (normalized == "warn" || normalized == "warning") return LogLevel::kWarn;
  if (normalized == "error") return LogLevel::kError;
  if (normalized == "critical" || normalized == "fatal")
    return LogLevel::kCritical;
  if (normalized == "off" || normalized == "none") return LogLevel::kOff;
  return fallback;
}

ConsoleSink::ConsoleSink(bool colorize)
    : ConsoleSink(std::cout, std::cerr, colorize) {}

ConsoleSink::ConsoleSink(std::ostream& out, std::ostream& err, bool colorize)
    : out_(out), err_(err), colorize_(colorize) {}

void ConsoleSink::Write(const LogMessage& message) {
  const auto line = ComposeLine(message);
  std::lock_guard lock(mutex_);
  auto& stream = message.level >= LogLevel::kError ? err_ : out_;
  if (colorize_) {
    stream << ColorForLevel(message.level) << line << "\033[0m" << std::endl;
  } else {
    stream << line << std::endl;
  }
}

void ConsoleSink::Flush() {
  std::lock_guard lock(mutex_);
  out_.flush();
  err_.flush();
}

FileSink::FileSink(std::filesystem::path path, bool append)
    : path_(std::move(path)), append_mode_(append) {
  Open(append_mode_);
}

void FileSink::Write(const LogMessage& message) {
  std::lock_guard lock(mutex_);
  if (!stream_.is_open()) {
    Open(append_mode_);
    if (!stream_.is_open()) return;
  }
  stream_ << ComposeLine(message) << '\n';
}

void FileSink::Flush() {
  std::lock_guard lock(mutex_);
  if (stream_.is_open()) stream_.flush();
}

bool FileSink::IsOpen() const {
  std::lock_guard lock(mutex_);
  return stream_.is_open();
}

void FileSink::Reopen(bool append) {
  std::lock_guard lock(mutex_);
  Open(append);
}

void FileSink::Open(bool append) {
  append_mode_ = append;
  std::ios::openmode mode = std::ios::out;
  if (append) {
    mode |= std::ios::app;
  } else {
    mode |= std::ios::trunc;
  }
  stream_.close();
  stream_.open(path_, mode);
}

Logger::Logger() = default;

Logger::Logger(std::string name) : name_(std::move(name)) {}

void Logger::SetLevel(LogLevel level) noexcept {
  level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::Level() const noexcept {
  return level_.load(std::memory_order_relaxed);
}

void Logger::AddSink(std::shared_ptr<LogSink> sink) {
  if (!sink) return;
  std::lock_guard lock(sinks_mutex_);
  sinks_.push_back(std::move(sink));
}

void Logger::RemoveSink(const std::shared_ptr<LogSink>& sink) {
  if (!sink) return;
  std::lock_guard lock(sinks_mutex_);
  sinks_.erase(std::remove(sinks_.begin(), sinks_.end(), sink), sinks_.end());
}

void Logger::ClearSinks() {
  std::lock_guard lock(sinks_mutex_);
  sinks_.clear();
}

void Logger::SetName(std::string name) {
  std::lock_guard lock(name_mutex_);
  name_ = std::move(name);
}

void Logger::Dispatch(LogMessage message) {
  std::vector<std::shared_ptr<LogSink>> sinks_copy;
  {
    std::lock_guard lock(sinks_mutex_);
    sinks_copy = sinks_;
  }
  for (const auto& sink : sinks_copy) {
    sink->Write(message);
  }
}

Logger& Logger::Default() {
  static Logger logger("engine");
  static std::once_flag once;
  std::call_once(once,
                 [&]() { logger.AddSink(std::make_shared<ConsoleSink>()); });
  return logger;
}

void Logger::ConfigureDefaultLevel(LogLevel level) noexcept {
  Default().SetLevel(level);
}

void Logger::AddDefaultSink(const std::shared_ptr<LogSink>& sink) {
  Default().AddSink(sink);
}

void Logger::ClearDefaultSinks() { Default().ClearSinks(); }

}  // namespace engine::util
