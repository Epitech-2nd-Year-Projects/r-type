#include "engine/scripting/script_engine.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "engine/util/logging.h"

namespace engine::scripting {

ScriptEngine::ScriptEngine() : lua_(std::make_unique<sol::state>()) {}

ScriptEngine::~ScriptEngine() = default;

ScriptEngine::ScriptEngine(ScriptEngine&&) noexcept = default;
ScriptEngine& ScriptEngine::operator=(ScriptEngine&&) noexcept = default;

void ScriptEngine::Initialize() {
  if (!lua_) {
    lua_ = std::make_unique<sol::state>();
  }

  lua_->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                       sol::lib::string, sol::lib::table, sol::lib::io,
                       sol::lib::os);

  lua_->set_function("log_info", [](const std::string& msg) {
    ENGINE_LOG_INFO("Lua: {}", msg);
  });

  lua_->set_function("log_error", [](const std::string& msg) {
    ENGINE_LOG_ERROR("Lua: {}", msg);
  });

  ENGINE_LOG_INFO("ScriptEngine initialized with Lua 5.4");
}

sol::state& ScriptEngine::LuaState() { return *lua_; }

}  // namespace engine::scripting
