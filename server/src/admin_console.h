#ifndef SERVER_ADMIN_CONSOLE_H_
#define SERVER_ADMIN_CONSOLE_H_

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace server {

class ServerRuntime;

/**
 * @brief Interactive admin console running in a separate thread.
 *
 * Provides a command-line interface for server administration with features:
 * - Command history navigation (arrow keys)
 * - Tab autocompletion for commands
 * - Thread-safe command execution via task queue
 *
 * Commands are executed on the main server thread to avoid data races.
 * The console uses linenoise for readline-like functionality.
 */
class AdminConsole {
 public:
  /**
   * @brief Constructs the admin console attached to a server runtime.
   * @param runtime Reference to the server runtime for command execution.
   */
  explicit AdminConsole(ServerRuntime& runtime);

  /**
   * @brief Destructor that ensures clean shutdown of the input thread.
   */
  ~AdminConsole();

  AdminConsole(const AdminConsole&) = delete;
  AdminConsole& operator=(const AdminConsole&) = delete;
  AdminConsole(AdminConsole&&) = delete;
  AdminConsole& operator=(AdminConsole&&) = delete;

  /**
   * @brief Start the console input thread.
   *
   * Loads command history from file and begins listening for user input.
   * Safe to call multiple times; subsequent calls are no-ops.
   */
  void Start();

  /**
   * @brief Stop the console input thread.
   *
   * Saves command history to file and signals the input thread to exit.
   * Safe to call multiple times; subsequent calls are no-ops.
   */
  void Stop();

 private:
  /**
   * @brief Represents a registered console command.
   */
  struct Command {
    std::string description;  ///< Help text describing the command.
    std::function<void(const std::vector<std::string>&)>
        handler;  ///< Handler function.
  };

  /**
   * @brief Main input loop running on the console thread.
   */
  void InputLoop();

  /**
   * @brief Register all available commands with their handlers.
   */
  void RegisterCommands();

  /**
   * @brief Parse and execute a command line.
   * @param line The raw input line from the user.
   */
  void ProcessLine(const std::string& line);

  /**
   * @brief Display the help message listing all commands.
   */
  void PrintHelp();

  /**
   * @brief Display an error message with formatting.
   * @param message The error message to display.
   */
  void PrintError(const std::string& message);

  /**
   * @brief Display a success message with formatting.
   * @param message The success message to display.
   */
  void PrintOk(const std::string& message);

  /**
   * @brief Display data in a formatted table.
   * @param rows The table rows, each row is a vector of cell values.
   * @param headers The column headers.
   */
  void PrintTable(const std::vector<std::vector<std::string>>& rows,
                  const std::vector<std::string>& headers);

  void CmdHelp(const std::vector<std::string>& args);
  void CmdStop(const std::vector<std::string>& args);
  void CmdStatus(const std::vector<std::string>& args);
  void CmdLogLevel(const std::vector<std::string>& args);
  void CmdQuiet(const std::vector<std::string>& args);
  void CmdVerbose(const std::vector<std::string>& args);
  void CmdPlayers(const std::vector<std::string>& args);
  void CmdKick(const std::vector<std::string>& args);
  void CmdWhisper(const std::vector<std::string>& args);
  void CmdRooms(const std::vector<std::string>& args);
  void CmdRoomInspect(const std::vector<std::string>& args);
  void CmdKillRoom(const std::vector<std::string>& args);
  void CmdBroadcast(const std::vector<std::string>& args);
  void CmdSpawn(const std::vector<std::string>& args);
  void CmdKillMobs(const std::vector<std::string>& args);
  void CmdListMobs(const std::vector<std::string>& args);

  ServerRuntime& runtime_;    ///< Reference to the server runtime.
  std::thread input_thread_;  ///< Thread running the input loop.
  std::atomic<bool> running_{
      false};  ///< Flag indicating if the console is active.
  std::unordered_map<std::string, Command>
      commands_;  ///< Registered command handlers.
  std::string history_file_ =
      ".rtype_console_history";  ///< Path to history file.
};

}  // namespace server

#endif  // SERVER_ADMIN_CONSOLE_H_
