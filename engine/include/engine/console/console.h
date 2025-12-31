#ifndef ENGINE_CONSOLE_CONSOLE_H_
#define ENGINE_CONSOLE_CONSOLE_H_

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "engine/console/command.h"
#include "engine/util/logging.h"

namespace engine::scripting {
class ScriptEngine;
}

namespace engine::console {

struct ConsoleLine {
  enum class Type { kInput, kOutput, kError, kInfo };

  Type type{Type::kOutput};
  std::string text;
};

struct ConsoleConfig {
  std::size_t max_history{1000};
  std::size_t max_input_history{100};
  bool capture_logs{true};
  util::LogLevel min_log_level{util::LogLevel::kInfo};
};

class ConsoleSink : public util::LogSink {
 public:
  using OutputCallback =
      std::function<void(const std::string&, ConsoleLine::Type)>;

  explicit ConsoleSink(OutputCallback callback,
                       util::LogLevel min_level = util::LogLevel::kInfo);

  void Write(const util::LogMessage& message) override;
  void SetMinLevel(util::LogLevel level) { min_level_ = level; }

 private:
  OutputCallback callback_;
  util::LogLevel min_level_;
  std::mutex mutex_;
};

class Console {
 public:
  Console();
  explicit Console(const ConsoleConfig& config);
  ~Console();

  Console(const Console&) = delete;
  Console& operator=(const Console&) = delete;
  Console(Console&&) = delete;
  Console& operator=(Console&&) = delete;

  void SetScriptEngine(scripting::ScriptEngine* script_engine);

  using QuitCallback = std::function<void()>;
  void SetQuitCallback(QuitCallback callback) {
    quit_callback_ = std::move(callback);
  }

  void Execute(std::string_view input);

  void Print(std::string_view text,
             ConsoleLine::Type type = ConsoleLine::Type::kOutput);
  void PrintInfo(std::string_view text);
  void PrintError(std::string_view text);

  void Clear();

  const std::deque<ConsoleLine>& GetHistory() const { return output_history_; }
  std::size_t GetHistorySize() const { return output_history_.size(); }

  const std::vector<std::string>& GetInputHistory() const {
    return input_history_;
  }
  void AddToInputHistory(std::string_view input);

  std::vector<std::string> Autocomplete(std::string_view partial) const;

  ConsoleConfig& config() { return config_; }
  const ConsoleConfig& config() const { return config_; }

  static Console& Instance();

 private:
  void Initialize();

  std::vector<std::string_view> Tokenize(std::string_view input) const;

  void HandleCVarAccess(std::string_view name,
                        std::span<const std::string_view> args);

  void ExecuteLua(std::string_view code);

  void RegisterConsoleCommands();

  void OnLogMessage(const std::string& text, ConsoleLine::Type type);

  ConsoleConfig config_;
  std::deque<ConsoleLine> output_history_;
  std::vector<std::string> input_history_;
  scripting::ScriptEngine* script_engine_{nullptr};
  std::shared_ptr<ConsoleSink> console_sink_;
  QuitCallback quit_callback_;
  mutable std::mutex mutex_;
};

}  // namespace engine::console

#endif  // ENGINE_CONSOLE_CONSOLE_H_
