#include "engine/scripting/script_engine.h"

#include <sol/sol.hpp>

#include "engine/scripting/bindings.h"
#include "engine/util/logging.h"

namespace engine::scripting {

ScriptEngine::ScriptEngine() = default;

ScriptEngine::~ScriptEngine() = default;

ScriptEngine::ScriptEngine(ScriptEngine&&) noexcept = default;
ScriptEngine& ScriptEngine::operator=(ScriptEngine&&) noexcept = default;

void ScriptEngine::Initialize() {
  if (!lua_) {
    lua_ = std::make_unique<sol::state>();
  }

  lua_->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                       sol::lib::string, sol::lib::table);

  lua_->set_function("log_info",
                     [](std::string msg) { ENGINE_LOG_INFO("Lua: {}", msg); });

  lua_->set_function("log_error",
                     [](std::string msg) { ENGINE_LOG_ERROR("Lua: {}", msg); });

  BindTypes(*lua_);

  ENGINE_LOG_INFO("ScriptEngine initialized with Lua 5.4");
}

void ScriptEngine::SetRegistry(engine::ecs::Registry& registry) {
  if (lua_) {
    BindRegistry(*lua_, registry);
  }
}

sol::state& ScriptEngine::LuaState() { return *lua_; }

}  // namespace engine::scripting
