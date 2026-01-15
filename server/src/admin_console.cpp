#include "admin_console.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "engine/util/logging.h"
#include "linenoise.hpp"
#include "protocol/command.h"
#include "server_runtime.h"

namespace server {

namespace {

std::vector<std::string> ParseArguments(const std::string& line) {
  std::vector<std::string> args;
  std::istringstream stream(line);
  std::string token;
  while (stream >> token) {
    args.push_back(token);
  }
  return args;
}

std::string JoinArgs(const std::vector<std::string>& args, std::size_t start) {
  std::ostringstream result;
  for (std::size_t i = start; i < args.size(); ++i) {
    if (i > start) {
      result << ' ';
    }
    result << args[i];
  }
  return result.str();
}

std::string FormatDuration(std::chrono::seconds total_seconds) {
  auto hours = std::chrono::duration_cast<std::chrono::hours>(total_seconds);
  auto minutes =
      std::chrono::duration_cast<std::chrono::minutes>(total_seconds - hours);
  auto seconds = total_seconds - hours - minutes;

  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << hours.count() << ":"
      << std::setfill('0') << std::setw(2) << minutes.count() << ":"
      << std::setfill('0') << std::setw(2) << seconds.count();
  return oss.str();
}

}  // namespace

AdminConsole::AdminConsole(ServerRuntime& runtime) : runtime_(runtime) {
  RegisterCommands();
}

AdminConsole::~AdminConsole() { Stop(); }

void AdminConsole::Start() {
  if (running_.exchange(true)) {
    return;
  }

  linenoise::LoadHistory(history_file_.c_str());

  linenoise::SetCompletionCallback(
      [this](const char* buf, std::vector<std::string>& completions) {
        std::string input(buf);
        for (const auto& [name, _] : commands_) {
          if (name.find(input) == 0) {
            completions.push_back(name);
          }
        }
      });

  input_thread_ = std::thread(&AdminConsole::InputLoop, this);
}

void AdminConsole::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

  linenoise::SaveHistory(history_file_.c_str());

  running_ = false;
  if (input_thread_.joinable()) {
    input_thread_.join();
  }
}

void AdminConsole::InputLoop() {
  std::string line;

  while (running_.load()) {
    bool quit = false;
    line = linenoise::Readline("rtype> ", quit);

    if (quit) {
      runtime_.RequestShutdown();
      break;
    }

    if (line.empty()) {
      continue;
    }

    linenoise::AddHistory(line.c_str());
    ProcessLine(line);
  }
}

void AdminConsole::RegisterCommands() {
  commands_["help"] = {"Display all available commands",
                       [this](const auto& args) { CmdHelp(args); }};

  commands_["stop"] = {"Gracefully shutdown the server",
                       [this](const auto& args) { CmdStop(args); }};

  commands_["exit"] = {"Gracefully shutdown the server (alias for stop)",
                       [this](const auto& args) { CmdStop(args); }};

  commands_["status"] = {"Show server status (tick rate, peers, rooms, uptime)",
                         [this](const auto& args) { CmdStatus(args); }};

  commands_["log_level"] = {
      "Change logging verbosity: log_level <info|debug|error>",
      [this](const auto& args) { CmdLogLevel(args); }};

  commands_["quiet"] = {"Disable console log output (file logs continue)",
                        [this](const auto& args) { CmdQuiet(args); }};

  commands_["verbose"] = {"Re-enable console log output",
                          [this](const auto& args) { CmdVerbose(args); }};

  commands_["players"] = {"List all connected players",
                          [this](const auto& args) { CmdPlayers(args); }};

  commands_["kick"] = {"Disconnect a player: kick <player_id>",
                       [this](const auto& args) { CmdKick(args); }};

  commands_["whisper"] = {"Send private message: whisper <player_id> <message>",
                          [this](const auto& args) { CmdWhisper(args); }};

  commands_["rooms"] = {"List all rooms",
                        [this](const auto& args) { CmdRooms(args); }};

  commands_["room_inspect"] = {
      "Show detailed room info: room_inspect <room_code>",
      [this](const auto& args) { CmdRoomInspect(args); }};

  commands_["kill_room"] = {"Force close a room: kill_room <room_code>",
                            [this](const auto& args) { CmdKillRoom(args); }};

  commands_["broadcast"] = {"Send message to all players: broadcast <message>",
                            [this](const auto& args) { CmdBroadcast(args); }};

  commands_["spawn"] = {"Spawn entity: spawn <room_code> <entity_type> <x> <y>",
                        [this](const auto& args) { CmdSpawn(args); }};

  commands_["kill_mobs"] = {
      "Remove all enemies in a room: kill_mobs <room_code>",
      [this](const auto& args) { CmdKillMobs(args); }};

  commands_["mobs"] = {"List available entity prefabs: mobs [room_code]",
                       [this](const auto& args) { CmdListMobs(args); }};

  commands_["list_mobs"] = commands_["mobs"];
}

void AdminConsole::ProcessLine(const std::string& line) {
  auto args = ParseArguments(line);
  if (args.empty()) {
    return;
  }

  const auto& cmd_name = args[0];
  auto it = commands_.find(cmd_name);
  if (it == commands_.end()) {
    PrintError("Unknown command: " + cmd_name +
               ". Type 'help' for available commands.");
    return;
  }

  it->second.handler(args);
}

void AdminConsole::PrintHelp() {
  std::cout << "\n\033[1;36m=== R-Type Server Admin Console ===\033[0m\n\n";

  std::vector<std::pair<std::string, std::string>> sorted_commands;
  for (const auto& [name, cmd] : commands_) {
    sorted_commands.emplace_back(name, cmd.description);
  }
  std::sort(sorted_commands.begin(), sorted_commands.end());

  for (const auto& [name, desc] : sorted_commands) {
    std::cout << "  \033[1;33m" << std::setw(14) << std::left << name
              << "\033[0m  " << desc << "\n";
  }
  std::cout << "\n";
}

void AdminConsole::PrintError(const std::string& message) {
  std::cout << "\033[1;31m[ERROR]\033[0m " << message << "\n";
}

void AdminConsole::PrintOk(const std::string& message) {
  std::cout << "\033[1;32m[OK]\033[0m " << message << "\n";
}

void AdminConsole::PrintTableV2(
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows) {
  if (rows.empty()) {
    return;
  }

  std::vector<std::size_t> widths(headers.size(), 0);
  for (std::size_t i = 0; i < headers.size(); ++i) {
    widths[i] = headers[i].size();
  }
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      widths[i] = std::max(widths[i], row[i].size());
    }
  }

  std::cout << "\033[1;36m";
  for (std::size_t i = 0; i < headers.size(); ++i) {
    std::cout << std::setw(static_cast<int>(widths[i] + 2)) << std::left
              << headers[i];
  }
  std::cout << "\033[0m\n";

  for (std::size_t i = 0; i < headers.size(); ++i) {
    std::cout << std::string(widths[i] + 2, '-');
  }
  std::cout << "\n";

  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      std::cout << std::setw(static_cast<int>(widths[i] + 2)) << std::left
                << row[i];
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

void AdminConsole::CmdHelp(const std::vector<std::string>&) { PrintHelp(); }

void AdminConsole::CmdStop(const std::vector<std::string>&) {
  PrintOk("Initiating graceful shutdown...");
  runtime_.RequestShutdown();
}

void AdminConsole::CmdStatus(const std::vector<std::string>&) {
  runtime_.EnqueueAdminTask([](ServerRuntime& rt) {
    auto now = std::chrono::steady_clock::now();
    auto uptime =
        std::chrono::duration_cast<std::chrono::seconds>(now - rt.StartTime());

    std::size_t peer_count = rt.Peers().size();
    std::size_t room_count = rt.Rooms().size();
    std::size_t player_count = rt.Players().size();
    std::uint32_t tick = rt.ServerTick();
    std::uint32_t tick_rate = rt.Config().tick_rate;

    std::cout << "\n\033[1;36m=== Server Status ===\033[0m\n"
              << "  Uptime:       " << FormatDuration(uptime) << "\n"
              << "  Tick:         " << tick << "\n"
              << "  Tick Rate:    " << tick_rate << " Hz\n"
              << "  Peers:        " << peer_count << "\n"
              << "  Players:      " << player_count << "\n"
              << "  Rooms:        " << room_count << "\n\n";
  });
}

void AdminConsole::CmdLogLevel(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: log_level <info|debug|error|warn|trace>");
    return;
  }

  const auto& level_str = args[1];
  auto level = engine::util::ParseLogLevel(level_str);

  runtime_.EnqueueAdminTask([level, level_str](ServerRuntime& rt) {
    rt.Logger().SetLevel(level);
    std::cout << "\033[1;32m[OK]\033[0m Log level set to: "
              << engine::util::ToString(level) << "\n";
  });
}

void AdminConsole::CmdQuiet(const std::vector<std::string>&) {
  runtime_.SetConsoleLogsEnabled(false);
  PrintOk("Console logging disabled. Logs continue to server.log");
}

void AdminConsole::CmdVerbose(const std::vector<std::string>&) {
  runtime_.SetConsoleLogsEnabled(true);
  PrintOk("Console logging enabled");
}

void AdminConsole::CmdPlayers(const std::vector<std::string>&) {
  runtime_.EnqueueAdminTask([this](ServerRuntime& rt) {
    const auto& peers = rt.Peers();
    const auto& players = rt.Players();

    if (players.empty()) {
      std::cout << "\033[1;33mNo players connected\033[0m\n\n";
      return;
    }

    std::vector<std::string> headers = {"ID", "Endpoint", "Room", "State"};
    std::vector<std::vector<std::string>> rows;

    for (const auto& [player_id, session] : players) {
      std::string state_str = "unknown";
      auto peer_it = peers.find(session.endpoint_key);
      if (peer_it != peers.end()) {
        switch (peer_it->second.state) {
          case PeerState::kConnecting:
            state_str = "connecting";
            break;
          case PeerState::kJoined:
            state_str = "joined";
            break;
          case PeerState::kDisconnected:
            state_str = "disconnected";
            break;
        }
      }

      rows.push_back({std::to_string(player_id), session.endpoint_key,
                      session.room_code, state_str});
    }

    PrintTableV2(headers, rows);
  });
}

void AdminConsole::CmdKick(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: kick <player_id>");
    return;
  }

  std::uint32_t player_id = 0;
  try {
    player_id = static_cast<std::uint32_t>(std::stoul(args[1]));
  } catch (...) {
    PrintError("Invalid player_id: " + args[1]);
    return;
  }

  runtime_.EnqueueAdminTask([player_id](ServerRuntime& rt) {
    if (rt.KickPlayer(player_id)) {
      std::cout << "\033[1;32m[OK]\033[0m Kicked player " << player_id << "\n"
                << std::flush;
    } else {
      std::cout << "\033[1;31m[ERROR]\033[0m Player " << player_id
                << " not found\n"
                << std::flush;
    }
  });
}

void AdminConsole::CmdWhisper(const std::vector<std::string>& args) {
  if (args.size() < 3) {
    PrintError("Usage: whisper <player_id> <message>");
    return;
  }

  std::uint32_t player_id = 0;
  try {
    player_id = static_cast<std::uint32_t>(std::stoul(args[1]));
  } catch (...) {
    PrintError("Invalid player_id: " + args[1]);
    return;
  }

  std::string message = JoinArgs(args, 2);

  runtime_.EnqueueAdminTask([player_id, message](ServerRuntime& rt) {
    const auto& players = rt.Players();
    auto it = players.find(player_id);
    if (it == players.end()) {
      std::cout << "\033[1;31m[ERROR]\033[0m Player " << player_id
                << " not found\n";
      return;
    }

    rt.Logger().Info("[AdminConsole] Whisper to player ", player_id, ": ",
                     message);
    std::cout << "\033[1;32m[OK]\033[0m Whispered to player " << player_id
              << ": " << message << "\n";
  });
}

void AdminConsole::CmdRooms(const std::vector<std::string>&) {
  runtime_.EnqueueAdminTask([this](ServerRuntime& rt) {
    const auto& rooms = rt.Rooms();

    if (rooms.empty()) {
      std::cout << "\033[1;33mNo active rooms\033[0m\n\n";
      return;
    }

    std::vector<std::string> headers = {"Code", "Name", "Players", "Privacy",
                                        "Started"};
    std::vector<std::vector<std::string>> rows;

    for (const auto& [code, room] : rooms) {
      std::string players_str = std::to_string(room.PlayerCount()) + "/" +
                                std::to_string(room.MaxPlayers());
      rows.push_back({room.Code(), room.Name(), players_str,
                      room.IsPrivate() ? "private" : "public",
                      room.HasStarted() ? "yes" : "no"});
    }

    PrintTableV2(headers, rows);
  });
}

void AdminConsole::CmdRoomInspect(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: room_inspect <room_code>");
    return;
  }

  std::string room_code = args[1];

  runtime_.EnqueueAdminTask([room_code](ServerRuntime& rt) {
    auto& rooms = rt.Rooms();
    auto it = rooms.find(room_code);
    if (it == rooms.end()) {
      std::cout << "\033[1;31m[ERROR]\033[0m Room '" << room_code
                << "' not found\n";
      return;
    }

    auto& room = it->second;

    std::cout << "\n\033[1;36m=== Room: " << room.Code() << " ===\033[0m\n"
              << "  Name:         " << room.Name() << "\n"
              << "  ID:           " << room.Id() << "\n"
              << "  Players:      " << room.PlayerCount() << "/"
              << room.MaxPlayers() << "\n"
              << "  Privacy:      " << (room.IsPrivate() ? "private" : "public")
              << "\n"
              << "  Started:      " << (room.HasStarted() ? "yes" : "no")
              << "\n"
              << "  Seed:         " << room.Seed() << "\n\n";
  });
}

void AdminConsole::CmdKillRoom(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: kill_room <room_code>");
    return;
  }

  std::string room_code = args[1];

  runtime_.EnqueueAdminTask([room_code](ServerRuntime& rt) {
    auto& rooms = rt.Rooms();
    auto it = rooms.find(room_code);
    if (it == rooms.end()) {
      std::cout << "\033[1;31m[ERROR]\033[0m Room '" << room_code
                << "' not found\n";
      return;
    }

    rooms.erase(it);
    std::cout << "\033[1;32m[OK]\033[0m Room '" << room_code
              << "' has been killed\n";
  });
}

void AdminConsole::CmdBroadcast(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: broadcast <message>");
    return;
  }

  std::string message = JoinArgs(args, 1);

  runtime_.EnqueueAdminTask([message](ServerRuntime& rt) {
    rt.Logger().Info("[AdminConsole] Broadcast: ", message);
    std::cout << "\033[1;32m[OK]\033[0m Broadcast sent: " << message << "\n";
  });
}

void AdminConsole::CmdSpawn(const std::vector<std::string>& args) {
  if (args.size() < 5) {
    PrintError("Usage: spawn <room_code> <entity_type> <x> <y>");
    return;
  }

  std::string room_code = args[1];
  std::string entity_type = args[2];
  float x = 0.0f;
  float y = 0.0f;

  try {
    x = std::stof(args[3]);
    y = std::stof(args[4]);
  } catch (...) {
    PrintError("Invalid coordinates");
    return;
  }

  runtime_.EnqueueAdminTask([room_code, entity_type, x, y](ServerRuntime& rt) {
    if (rt.SpawnEntityInRoom(room_code, entity_type, x, y)) {
      std::cout << "\033[1;32m[OK]\033[0m Spawned '" << entity_type << "' at ("
                << x << ", " << y << ") in room " << room_code << "\n"
                << std::flush;
    } else {
      std::cout << "\033[1;31m[ERROR]\033[0m Failed to spawn '" << entity_type
                << "' (room not found or invalid prefab)\n"
                << std::flush;
    }
  });
}

void AdminConsole::CmdKillMobs(const std::vector<std::string>& args) {
  if (args.size() < 2) {
    PrintError("Usage: kill_mobs <room_code>");
    return;
  }

  std::string room_code = args[1];

  runtime_.EnqueueAdminTask([room_code](ServerRuntime& rt) {
    std::size_t count = rt.RemoveEnemiesFromRoom(room_code);
    if (count > 0) {
      std::cout << "\033[1;32m[OK]\033[0m Removed " << count
                << " enemies from room '" << room_code << "'\n"
                << std::flush;
    } else {
      std::cout << "\033[1;33m[WARN]\033[0m No enemies found in room '"
                << room_code << "' (or room not found)\n"
                << std::flush;
    }
  });
}

void AdminConsole::CmdListMobs(const std::vector<std::string>& args) {
  std::string room_code;
  if (args.size() > 1) {
    room_code = args[1];
  }

  runtime_.EnqueueAdminTask([this, room_code](ServerRuntime& rt) {
    std::string target_room = room_code;
    auto& rooms = rt.Rooms();

    if (target_room.empty()) {
      if (rooms.empty()) {
        std::cout << "\033[1;33m[WARN]\033[0m No active rooms to query "
                     "available mobs from.\n"
                  << std::flush;
        return;
      }
      target_room = rooms.begin()->first;
    }

    std::vector<std::string> prefabs = rt.GetAvailableEntities(target_room);
    if (prefabs.empty()) {
      if (rooms.find(target_room) == rooms.end()) {
        std::cout << "\033[1;31m[ERROR]\033[0m Room '" << target_room
                  << "' not found.\n"
                  << std::flush;
      } else {
        std::cout << "\033[1;33m[WARN]\033[0m No prefabs found in room '"
                  << target_room << "'.\n"
                  << std::flush;
      }
      return;
    }

    std::vector<std::vector<std::string>> rows;
    for (const auto& name : prefabs) {
      rows.push_back({name});
    }
    std::vector<std::string> headers = {"Available Prefabs"};
    PrintTableV2(headers, rows);
    std::cout << std::flush;
  });
}

}  // namespace server
