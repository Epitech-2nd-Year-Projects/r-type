#ifndef ENGINE_CONSOLE_COMMAND_H_
#define ENGINE_CONSOLE_COMMAND_H_

#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine::console {

struct CommandResult {
  bool success{true};
  std::string output;

  static CommandResult Ok(std::string message = "") {
    return {true, std::move(message)};
  }

  static CommandResult Error(std::string message) {
    return {false, std::move(message)};
  }
};

using CommandHandler = std::function<CommandResult(std::span<const std::string_view> args)>;
using AutocompleteHandler = std::function<std::vector<std::string>(std::string_view partial)>;

struct Command {
  std::string name;
  std::string help;
  CommandHandler handler;
  AutocompleteHandler autocomplete;
};

class CommandRegistry {
 public:
  static CommandRegistry& Instance();

  void Register(std::string name, CommandHandler handler,
                std::string help = "");

  void Register(std::string name, CommandHandler handler,
                AutocompleteHandler autocomplete, std::string help = "");

  void Unregister(std::string_view name);

  bool Exists(std::string_view name) const;

  const Command* Find(std::string_view name) const;

  CommandResult Execute(std::string_view name,
                        std::span<const std::string_view> args);

  std::vector<std::string> GetAllNames() const;

  std::vector<std::string> GetNamesWithPrefix(std::string_view prefix) const;

  std::vector<std::string> Autocomplete(std::string_view input) const;

  void Clear();

 private:
  CommandRegistry();

  void RegisterBuiltins();

  std::unordered_map<std::string, Command> commands_;
  mutable std::mutex mutex_;
};

}  // namespace engine::console

#endif  // ENGINE_CONSOLE_COMMAND_H_

