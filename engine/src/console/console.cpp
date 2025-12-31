#include "engine/console/console.h"

#include <algorithm>
#include <sol/sol.hpp>
#include <sstream>

#include "engine/console/cvar.h"
#include "engine/scripting/script_engine.h"

namespace engine::console {

ConsoleSink::ConsoleSink(OutputCallback callback, util::LogLevel min_level)
    : callback_(std::move(callback)), min_level_(min_level) {}

void ConsoleSink::Write(const util::LogMessage& message) {
  if (message.level < min_level_) {
    return;
  }

  std::lock_guard lock(mutex_);

  ConsoleLine::Type type = ConsoleLine::Type::kInfo;
  if (message.level >= util::LogLevel::kError) {
    type = ConsoleLine::Type::kError;
  }

  std::ostringstream oss;
  oss << "[" << util::ToString(message.level) << "] " << message.text;

  if (callback_) {
    callback_(oss.str(), type);
  }
}

Console::Console() : Console(ConsoleConfig{}) {}

Console::Console(const ConsoleConfig& config) : config_(config) {
  Initialize();
}

Console::~Console() {
  if (console_sink_) {
    util::Logger::Default().RemoveSink(console_sink_);
  }
}

void Console::Initialize() {
  RegisterConsoleCommands();

  if (config_.capture_logs) {
    console_sink_ = std::make_shared<ConsoleSink>(
        [this](const std::string& text, ConsoleLine::Type type) {
          OnLogMessage(text, type);
        },
        config_.min_log_level);

    util::Logger::AddDefaultSink(console_sink_);
  }
}

void Console::SetScriptEngine(scripting::ScriptEngine* script_engine) {
  script_engine_ = script_engine;

  if (script_engine_) {
    auto& lua = script_engine_->LuaState();

    lua.set_function("cvar_get", [](const std::string& name) -> std::string {
      return CVarRegistry::Instance().GetString(name);
    });

    lua.set_function(
        "cvar_set",
        [](const std::string& name, const std::string& value) -> bool {
          return CVarRegistry::Instance().SetFromString(name, value);
        });

    lua.set_function("cvar_exists", [](const std::string& name) -> bool {
      return CVarRegistry::Instance().Exists(name);
    });

    lua.set_function("console_exec",
                     [this](const std::string& cmd) { Execute(cmd); });

    lua.set_function("console_print",
                     [this](const std::string& text) { Print(text); });
  }
}

void Console::Execute(std::string_view input) {
  if (input.empty()) {
    return;
  }

  std::string input_str(input);
  AddToInputHistory(input_str);
  Print("> " + input_str, ConsoleLine::Type::kInput);

  if (input.starts_with("lua ")) {
    ExecuteLua(input.substr(4));
    return;
  }

  if (input.front() == '`' && input.back() == '`' && input.size() > 2) {
    ExecuteLua(input.substr(1, input.size() - 2));
    return;
  }

  auto tokens = Tokenize(input);
  if (tokens.empty()) {
    return;
  }

  std::string_view command = tokens[0];
  std::vector<std::string_view> args(tokens.begin() + 1, tokens.end());

  if (CVarRegistry::Instance().Exists(command)) {
    HandleCVarAccess(command, args);
    return;
  }

  if (CommandRegistry::Instance().Exists(command)) {
    auto result = CommandRegistry::Instance().Execute(command, args);
    if (!result.output.empty()) {
      Print(result.output, result.success ? ConsoleLine::Type::kOutput
                                          : ConsoleLine::Type::kError);
    }
    return;
  }

  PrintError("Unknown command or cvar: " + std::string(command));
}

void Console::HandleCVarAccess(std::string_view name,
                               std::span<const std::string_view> args) {
  auto& registry = CVarRegistry::Instance();

  if (args.empty()) {
    auto* cvar = registry.Find(name);
    if (cvar) {
      std::ostringstream oss;
      oss << name << " = " << cvar->GetString();
      if (!cvar->description().empty()) {
        oss << " (" << cvar->description() << ")";
      }
      Print(oss.str());
    }
    return;
  }

  std::string value(args[0]);
  if (registry.SetFromString(name, value)) {
    Print(std::string(name) + " = " + registry.GetString(name));
  } else {
    PrintError("Failed to set " + std::string(name));
  }
}

void Console::ExecuteLua(std::string_view code) {
  if (!script_engine_) {
    PrintError("Script engine not available");
    return;
  }

  try {
    auto& lua = script_engine_->LuaState();
    auto result = lua.safe_script(std::string(code), sol::script_pass_on_error);

    if (!result.valid()) {
      sol::error err = result;
      PrintError(err.what());
    } else if (result.get_type() != sol::type::none &&
               result.get_type() != sol::type::nil) {
      std::ostringstream oss;
      sol::object obj = result;

      if (obj.is<std::string>()) {
        oss << obj.as<std::string>();
      } else if (obj.is<double>()) {
        oss << obj.as<double>();
      } else if (obj.is<bool>()) {
        oss << (obj.as<bool>() ? "true" : "false");
      } else {
        oss << "["
            << lua_typename(lua.lua_state(),
                            static_cast<int>(result.get_type()))
            << "]";
      }

      Print(oss.str());
    }
  } catch (const std::exception& e) {
    PrintError(e.what());
  }
}

void Console::Print(std::string_view text, ConsoleLine::Type type) {
  std::lock_guard lock(mutex_);

  output_history_.push_back({type, std::string(text)});

  while (output_history_.size() > config_.max_history) {
    output_history_.pop_front();
  }
}

void Console::PrintInfo(std::string_view text) {
  Print(text, ConsoleLine::Type::kInfo);
}

void Console::PrintError(std::string_view text) {
  Print(text, ConsoleLine::Type::kError);
}

void Console::Clear() {
  std::lock_guard lock(mutex_);
  output_history_.clear();
}

void Console::AddToInputHistory(std::string_view input) {
  std::lock_guard lock(mutex_);

  if (!input_history_.empty() && input_history_.back() == input) {
    return;
  }

  input_history_.emplace_back(input);

  while (input_history_.size() > config_.max_input_history) {
    input_history_.erase(input_history_.begin());
  }
}

std::vector<std::string> Console::Autocomplete(std::string_view partial) const {
  std::vector<std::string> results;

  auto cmd_completions = CommandRegistry::Instance().Autocomplete(partial);
  results.insert(results.end(), cmd_completions.begin(), cmd_completions.end());

  if (partial.find(' ') == std::string_view::npos) {
    auto cvar_completions =
        CVarRegistry::Instance().GetNamesWithPrefix(partial);
    results.insert(results.end(), cvar_completions.begin(),
                   cvar_completions.end());
  }

  std::sort(results.begin(), results.end());
  results.erase(std::unique(results.begin(), results.end()), results.end());

  return results;
}

std::vector<std::string_view> Console::Tokenize(std::string_view input) const {
  std::vector<std::string_view> tokens;

  std::size_t i = 0;
  while (i < input.size()) {
    while (i < input.size() &&
           std::isspace(static_cast<unsigned char>(input[i]))) {
      ++i;
    }

    if (i >= input.size()) {
      break;
    }

    if (input[i] == '"') {
      ++i;
      std::size_t start = i;
      while (i < input.size() && input[i] != '"') {
        ++i;
      }
      tokens.push_back(input.substr(start, i - start));
      if (i < input.size()) {
        ++i;
      }
    } else {
      std::size_t start = i;
      while (i < input.size() &&
             !std::isspace(static_cast<unsigned char>(input[i]))) {
        ++i;
      }
      tokens.push_back(input.substr(start, i - start));
    }
  }

  return tokens;
}

void Console::RegisterConsoleCommands() {
  CommandRegistry::Instance().Register(
      "clear",
      [this](std::span<const std::string_view>) -> CommandResult {
        Clear();
        return CommandResult::Ok();
      },
      "clear - Clear console output");

  CommandRegistry::Instance().Register(
      "quit",
      [this](std::span<const std::string_view>) -> CommandResult {
        if (quit_callback_) {
          quit_callback_();
          return CommandResult::Ok();
        }
        std::exit(0);
        return CommandResult::Ok();
      },
      "quit - Exit the application");

  CommandRegistry::Instance().Register(
      "exec",
      [this](std::span<const std::string_view> args) -> CommandResult {
        if (args.empty()) {
          return CommandResult::Error("Usage: exec <script.lua>");
        }

        if (!script_engine_) {
          return CommandResult::Error("Script engine not available");
        }

        std::string path(args[0]);
        try {
          script_engine_->LoadScript(path);
          return CommandResult::Ok("Executed: " + path);
        } catch (const std::exception& e) {
          return CommandResult::Error(e.what());
        }
      },
      "exec <file> - Execute a Lua script file");
}

void Console::OnLogMessage(const std::string& text, ConsoleLine::Type type) {
  std::lock_guard lock(mutex_);

  output_history_.push_back({type, text});

  while (output_history_.size() > config_.max_history) {
    output_history_.pop_front();
  }
}

Console& Console::Instance() {
  static Console instance;
  return instance;
}

}  // namespace engine::console
