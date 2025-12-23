#ifndef ENGINE_SCRIPTING_SCRIPT_ENGINE_H_
#define ENGINE_SCRIPTING_SCRIPT_ENGINE_H_

#include <memory>
#include <string>
#include <string_view>

namespace sol {
class state;
}

namespace engine::scripting {

/**
 * @class ScriptEngine
 * @brief Manages the Lua scripting environment
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
   * @brief Initialize the Lua state and open standard libraries
   */
  void Initialize();

  /**
   * @brief Access the underlying Lua state
   */
  sol::state& LuaState();

 private:
  std::unique_ptr<sol::state> lua_;
};

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_SCRIPT_ENGINE_H_
