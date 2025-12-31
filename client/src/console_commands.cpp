#include "console_commands.h"

#include <iomanip>
#include <sstream>

#include "application.h"
#include "client_runtime.h"
#include "ecs/components.h"
#include "engine/console/command.h"
#include "engine/console/cvar.h"
#include "engine/ecs/registry.h"
#include "input/key_bindings.h"

namespace client {

namespace {

std::string EntityTypeString(std::uint16_t type_code) {
  switch (type_code) {
    case 1:
      return "Player";
    case 2:
      return "Enemy";
    case 3:
      return "Missile";
    case 4:
      return "PowerUp";
    default:
      return "Unknown(" + std::to_string(type_code) + ")";
  }
}

}  // namespace

void RegisterConsoleCommands(Application& app) {
  auto& cmd_registry = engine::console::CommandRegistry::Instance();
  auto& cvar_registry = engine::console::CVarRegistry::Instance();

  cmd_registry.Register(
      "commands",
      [&cmd_registry](std::span<const std::string_view>)
          -> engine::console::CommandResult {
        auto names = cmd_registry.GetAllNames();
        std::ostringstream oss;
        oss << "Registered commands (" << names.size() << "):\n";
        for (const auto& name : names) {
          oss << "  " << name << "\n";
        }
        return engine::console::CommandResult::Ok(oss.str());
      },
      "commands - List all registered commands");

  cvar_registry.Register<bool>("cl_show_fps", true, "Show FPS counter");
  cvar_registry.Register<float>("cl_volume", 1.0f, 0.0f, 1.0f, "Master volume");
  cvar_registry.Register<bool>("sv_cheats", false, "Enable cheat commands",
                               engine::console::CVarFlags::kCheat);

  cmd_registry.Register(
      "entities",
      [&app](std::span<const std::string_view> args)
          -> engine::console::CommandResult {
        const auto& registry = app.World();
        const auto& networked =
            registry.GetComponents<ecs::NetworkedEntityComponent>();
        const auto& positions = registry.GetComponents<ecs::PositionComponent>();
        const auto& health = registry.GetComponents<ecs::HealthComponent>();

        std::optional<std::uint32_t> filter_id;
        if (!args.empty()) {
          try {
            filter_id = static_cast<std::uint32_t>(std::stoul(std::string(args[0])));
          } catch (...) {
            return engine::console::CommandResult::Error(
                "Invalid entity ID: " + std::string(args[0]));
          }
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);

        std::size_t count = 0;
        for (std::size_t i = 0; i < networked.size(); ++i) {
          if (!networked[i].has_value()) continue;

          const auto& net = *networked[i];
          if (filter_id.has_value() && net.network_id != *filter_id) continue;

          oss << "Entity #" << net.network_id << " ["
              << EntityTypeString(net.type_code) << "]";

          if (i < positions.size() && positions[i].has_value()) {
            const auto& pos = positions[i]->position;
            oss << " pos=(" << pos.x << ", " << pos.y << ")";
          }

          if (i < health.size() && health[i].has_value()) {
            oss << " hp=" << static_cast<int>(health[i]->current) << "/"
                << static_cast<int>(health[i]->max);
          }

          oss << "\n";
          ++count;

          if (count >= 20 && !filter_id.has_value()) {
            oss << "... and more (use 'entities <id>' for specific entity)\n";
            break;
          }
        }

        if (count == 0) {
          return engine::console::CommandResult::Ok("No entities found");
        }

        oss << "Total: " << count << " entities";
        return engine::console::CommandResult::Ok(oss.str());
      },
      "entities [id] - List entities or show details for a specific entity");

  cmd_registry.Register(
      "bind",
      [&app](std::span<const std::string_view> args)
          -> engine::console::CommandResult {
        if (args.size() < 2) {
          std::ostringstream oss;
          oss << "Current bindings:\n";
          const auto& bindings = app.KeyBindingSet();
          for (auto action : bindings.Actions()) {
            auto key = bindings.Primary(action);
            oss << "  " << ActionLabel(action) << " = "
                << KeyDisplayName(key) << "\n";
          }
          oss << "\nUsage: bind <action> <key>\n";
          oss << "Actions: move_up, move_down, move_left, move_right, shoot, "
                 "big_shoot, reconnect";
          return engine::console::CommandResult::Ok(oss.str());
        }

        std::string action_str(args[0]);
        std::string key_str(args[1]);

        GameAction action;
        if (action_str == "move_up") {
          action = GameAction::kMoveUp;
        } else if (action_str == "move_down") {
          action = GameAction::kMoveDown;
        } else if (action_str == "move_left") {
          action = GameAction::kMoveLeft;
        } else if (action_str == "move_right") {
          action = GameAction::kMoveRight;
        } else if (action_str == "shoot") {
          action = GameAction::kShoot;
        } else if (action_str == "big_shoot") {
          action = GameAction::kBigShoot;
        } else if (action_str == "reconnect") {
          action = GameAction::kReconnect;
        } else {
          return engine::console::CommandResult::Error(
              "Unknown action: " + action_str);
        }

        auto key = ParseKeyToken(key_str);
        if (!key.has_value()) {
          return engine::console::CommandResult::Error(
              "Unknown key: " + key_str);
        }

        auto result = app.UpdateKeyBinding(action, *key);
        if (result.status == KeyBindingUpdateStatus::kUpdated) {
          return engine::console::CommandResult::Ok(result.message);
        }
        return engine::console::CommandResult::Error(result.message);
      },
      [](std::string_view partial) -> std::vector<std::string> {
        std::vector<std::string> suggestions;
        std::vector<std::string> actions = {"move_up",   "move_down",
                                            "move_left", "move_right",
                                            "shoot",     "big_shoot",
                                            "reconnect"};
        for (const auto& action : actions) {
          if (action.find(partial) == 0) {
            suggestions.push_back(action);
          }
        }
        return suggestions;
      },
      "bind <action> <key> - Bind a key to an action");

  cmd_registry.Register(
      "player",
      [&app](std::span<const std::string_view>)
          -> engine::console::CommandResult {
        auto player_id = app.LocalPlayerId();
        if (!player_id.has_value()) {
          return engine::console::CommandResult::Error("Not connected");
        }

        const auto& registry = app.World();
        const auto& networked =
            registry.GetComponents<ecs::NetworkedEntityComponent>();
        const auto& positions = registry.GetComponents<ecs::PositionComponent>();
        const auto& health = registry.GetComponents<ecs::HealthComponent>();
        const auto& player_states =
            registry.GetComponents<ecs::PlayerStateComponent>();

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);

        for (std::size_t i = 0; i < networked.size(); ++i) {
          if (!networked[i].has_value()) continue;
          if (networked[i]->network_id != *player_id) continue;

          oss << "Local Player #" << *player_id << "\n";

          if (i < positions.size() && positions[i].has_value()) {
            const auto& pos = positions[i]->position;
            oss << "  Position: (" << pos.x << ", " << pos.y << ")\n";
          }

          if (i < health.size() && health[i].has_value()) {
            oss << "  Health: " << static_cast<int>(health[i]->current) << "/"
                << static_cast<int>(health[i]->max) << "\n";
          }

          if (i < player_states.size() && player_states[i].has_value()) {
            const auto& state = *player_states[i];
            oss << "  Score: " << state.score << "\n";
            oss << "  Lives: " << static_cast<int>(state.lives) << "\n";
          }

          return engine::console::CommandResult::Ok(oss.str());
        }

        return engine::console::CommandResult::Error("Player entity not found");
      },
      "player - Show local player info");

  cmd_registry.Register(
      "wave",
      [&app](std::span<const std::string_view>)
          -> engine::console::CommandResult {
        auto wave = app.CurrentWave();
        if (!wave.has_value()) {
          return engine::console::CommandResult::Error("Not in game");
        }
        return engine::console::CommandResult::Ok("Current wave: " +
                                                  std::to_string(*wave));
      },
      "wave - Show current wave number");

  cmd_registry.Register(
      "ping",
      [&app](std::span<const std::string_view>)
          -> engine::console::CommandResult {
        auto latency = app.LatestLatencyMs();
        if (!latency.has_value()) {
          return engine::console::CommandResult::Error("Not connected");
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "Latency: " << *latency << " ms";
        return engine::console::CommandResult::Ok(oss.str());
      },
      "ping - Show current network latency");

  cmd_registry.Register(
      "disconnect",
      [&app](std::span<const std::string_view>)
          -> engine::console::CommandResult {
        if (!app.ConnectionActive()) {
          return engine::console::CommandResult::Error("Not connected");
        }
        app.OnQuitToMenu();
        return engine::console::CommandResult::Ok("Disconnected");
      },
      "disconnect - Disconnect from server and return to menu");
}

}  // namespace client

