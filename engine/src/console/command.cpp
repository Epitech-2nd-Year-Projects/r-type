#include "engine/console/command.h"

#include <algorithm>
#include <sstream>

#include "engine/console/cvar.h"

namespace engine::console {

CommandRegistry& CommandRegistry::Instance() {
  static CommandRegistry instance;
  return instance;
}

CommandRegistry::CommandRegistry() {
  RegisterBuiltins();
}

void CommandRegistry::RegisterBuiltins() {
  Register(
      "help",
      [this](std::span<const std::string_view> args) -> CommandResult {
        if (args.empty()) {
          std::ostringstream oss;
          oss << "Available commands:\n";
          auto names = GetAllNames();
          for (const auto& name : names) {
            const auto* cmd = Find(name);
            if (cmd && !cmd->help.empty()) {
              oss << "  " << name << " - " << cmd->help << "\n";
            } else {
              oss << "  " << name << "\n";
            }
          }
          return CommandResult::Ok(oss.str());
        }

        std::string cmd_name(args[0]);
        const auto* cmd = Find(cmd_name);
        if (!cmd) {
          return CommandResult::Error("Unknown command: " + cmd_name);
        }
        if (cmd->help.empty()) {
          return CommandResult::Ok(cmd_name + ": No help available");
        }
        return CommandResult::Ok(cmd->help);
      },
      "help [command] - Show help for commands");

  Register(
      "echo",
      [](std::span<const std::string_view> args) -> CommandResult {
        std::ostringstream oss;
        for (std::size_t i = 0; i < args.size(); ++i) {
          if (i > 0) oss << " ";
          oss << args[i];
        }
        return CommandResult::Ok(oss.str());
      },
      "echo <text> - Print text to console");

  Register(
      "list_cvars",
      [](std::span<const std::string_view> args) -> CommandResult {
        std::string prefix;
        if (!args.empty()) {
          prefix = std::string(args[0]);
        }

        auto& registry = CVarRegistry::Instance();
        auto names = prefix.empty() ? registry.GetAllNames()
                                    : registry.GetNamesWithPrefix(prefix);

        if (names.empty()) {
          return CommandResult::Ok("No CVars found");
        }

        std::ostringstream oss;
        oss << "Console Variables:\n";
        for (const auto& name : names) {
          const auto* cvar = registry.Find(name);
          if (cvar) {
            oss << "  " << name << " = " << cvar->GetString();
            if (!cvar->description().empty()) {
              oss << " (" << cvar->description() << ")";
            }
            oss << "\n";
          }
        }
        return CommandResult::Ok(oss.str());
      },
      [](std::string_view partial) -> std::vector<std::string> {
        return CVarRegistry::Instance().GetNamesWithPrefix(partial);
      },
      "list_cvars [prefix] - List all console variables");

  Register(
      "set",
      [](std::span<const std::string_view> args) -> CommandResult {
        if (args.size() < 2) {
          return CommandResult::Error("Usage: set <cvar> <value>");
        }

        std::string name(args[0]);
        std::string value(args[1]);

        auto& registry = CVarRegistry::Instance();
        if (!registry.Exists(name)) {
          return CommandResult::Error("Unknown CVar: " + name);
        }

        if (!registry.SetFromString(name, value)) {
          return CommandResult::Error("Failed to set " + name);
        }

        return CommandResult::Ok(name + " = " + registry.GetString(name));
      },
      [](std::string_view partial) -> std::vector<std::string> {
        return CVarRegistry::Instance().GetNamesWithPrefix(partial);
      },
      "set <cvar> <value> - Set a console variable");

  Register(
      "get",
      [](std::span<const std::string_view> args) -> CommandResult {
        if (args.empty()) {
          return CommandResult::Error("Usage: get <cvar>");
        }

        std::string name(args[0]);
        auto& registry = CVarRegistry::Instance();

        const auto* cvar = registry.Find(name);
        if (!cvar) {
          return CommandResult::Error("Unknown CVar: " + name);
        }

        return CommandResult::Ok(name + " = " + cvar->GetString());
      },
      [](std::string_view partial) -> std::vector<std::string> {
        return CVarRegistry::Instance().GetNamesWithPrefix(partial);
      },
      "get <cvar> - Get a console variable value");

  Register(
      "reset",
      [](std::span<const std::string_view> args) -> CommandResult {
        if (args.empty()) {
          return CommandResult::Error("Usage: reset <cvar>");
        }

        std::string name(args[0]);
        auto* cvar = CVarRegistry::Instance().Find(name);
        if (!cvar) {
          return CommandResult::Error("Unknown CVar: " + name);
        }

        cvar->Reset();
        return CommandResult::Ok(name + " reset to " + cvar->GetString());
      },
      [](std::string_view partial) -> std::vector<std::string> {
        return CVarRegistry::Instance().GetNamesWithPrefix(partial);
      },
      "reset <cvar> - Reset a console variable to default");
}

void CommandRegistry::Register(std::string name, CommandHandler handler,
                               std::string help) {
  std::lock_guard lock(mutex_);
  commands_[name] = Command{name, std::move(help),
                            std::move(handler), nullptr};
}

void CommandRegistry::Register(std::string name, CommandHandler handler,
                               AutocompleteHandler autocomplete,
                               std::string help) {
  std::lock_guard lock(mutex_);
  commands_[name] = Command{name, std::move(help),
                            std::move(handler), std::move(autocomplete)};
}

void CommandRegistry::Unregister(std::string_view name) {
  std::lock_guard lock(mutex_);
  commands_.erase(std::string(name));
}

bool CommandRegistry::Exists(std::string_view name) const {
  std::lock_guard lock(mutex_);
  return commands_.find(std::string(name)) != commands_.end();
}

const Command* CommandRegistry::Find(std::string_view name) const {
  std::lock_guard lock(mutex_);
  auto it = commands_.find(std::string(name));
  return it != commands_.end() ? &it->second : nullptr;
}

CommandResult CommandRegistry::Execute(std::string_view name,
                                        std::span<const std::string_view> args) {
  const Command* cmd = Find(name);
  if (!cmd) {
    return CommandResult::Error("Unknown command: " + std::string(name));
  }
  return cmd->handler(args);
}

std::vector<std::string> CommandRegistry::GetAllNames() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  names.reserve(commands_.size());
  for (const auto& [name, _] : commands_) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> CommandRegistry::GetNamesWithPrefix(
    std::string_view prefix) const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  for (const auto& [name, _] : commands_) {
    if (name.size() >= prefix.size() &&
        name.compare(0, prefix.size(), prefix) == 0) {
      names.push_back(name);
    }
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> CommandRegistry::Autocomplete(
    std::string_view input) const {
  if (input.empty()) {
    return GetAllNames();
  }

  auto space_pos = input.find(' ');
  if (space_pos == std::string_view::npos) {
    return GetNamesWithPrefix(input);
  }

  std::string cmd_name(input.substr(0, space_pos));
  std::string_view rest = input.substr(space_pos + 1);

  const Command* cmd = Find(cmd_name);
  if (!cmd || !cmd->autocomplete) {
    return {};
  }

  auto completions = cmd->autocomplete(rest);
  std::vector<std::string> result;
  result.reserve(completions.size());
  for (auto& completion : completions) {
    result.push_back(cmd_name + " " + completion);
  }
  return result;
}

void CommandRegistry::Clear() {
  std::lock_guard lock(mutex_);
  commands_.clear();
  RegisterBuiltins();
}

}  // namespace engine::console

