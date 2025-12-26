#ifndef ENGINE_SCRIPTING_SCRIPT_ENGINE_H_
#define ENGINE_SCRIPTING_SCRIPT_ENGINE_H_

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sol {
class state;
}

namespace engine::ecs {
class Registry;
}

namespace engine::event {
class EventBus;
}

namespace engine::scripting {

/**
 * @class ScriptEngine
 * @brief Manages the Lua scripting environment.
 *
 * @note This class is NOT thread-safe for write operations.
 * @note Lua 5.4 is used as the scripting language.
 * @note Standard libraries 'io' and 'os' are disabled for security.
 */
class ScriptEngine {
 public:
  ScriptEngine();
  ~ScriptEngine();

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;
  ScriptEngine(ScriptEngine&&) noexcept;
  ScriptEngine& operator=(ScriptEngine&&) noexcept;

  /**
   * @brief Initialize the Lua state and open standard libraries.
   *
   * @note Must be called before accessing LuaState().
   */
  void Initialize();

  /**
   * @brief Bind the global registry instance.
   * @param registry Reference to the registry.
   *
   * @note The registry reference must remain valid for the lifetime of
   * the ScriptEngine or until unbound. The ScriptEngine does not take
   * ownership of the registry.
   */
  void SetRegistry(engine::ecs::Registry& registry);

  /**
   * @brief Bind the global event bus.
   * @param event_bus Reference to the event bus.
   *
   * @note The event_bus reference must remain valid for the lifetime of
   * the ScriptEngine or until unbound. The ScriptEngine does not take
   * ownership of the event_bus.
   */
  void SetEventBus(engine::event::EventBus& event_bus);

  /**
   * @brief Access the underlying Lua state.
   *
   * @warning Exposes raw mutable state. Use with caution.
   *          Modifying the state (e.g. closing libraries) may break engine
   * assumptions.
   */
  sol::state& LuaState();

  /**
   * @brief Load and execute a script file, then monitor it for changes
   * (hot-reload).
   * @param path Absolute or relative path to the .lua file.
   */
  void LoadScript(const std::string& path);

  /**
   * @brief Check for file changes and reload modified scripts.
   * Call this once per frame or at a regular interval.
   */
  void Update();

 private:
  struct WatchedScript {
    std::string path;
    std::filesystem::file_time_type last_write_time;
  };

  std::unique_ptr<sol::state> lua_;
  std::vector<WatchedScript> watched_scripts_;
  void ReloadScript(const std::string& path);
};

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_SCRIPT_ENGINE_H_
