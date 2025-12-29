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

  prefab_factory_ = std::make_unique<PrefabFactory>(*lua_);

  ENGINE_LOG_INFO("ScriptEngine initialized with Lua 5.4");
}

void ScriptEngine::SetRegistry(engine::ecs::Registry& registry) {
  if (lua_) {
    BindRegistry(*lua_, registry);
  }
}

void ScriptEngine::SetEventBus(engine::event::EventBus& event_bus) {
  if (lua_) {
    BindEventBus(*lua_, event_bus);
  }
}

void ScriptEngine::SetInputManager(engine::input::InputManager& input_manager) {
  if (lua_) {
    BindInput(*lua_, input_manager);
  }
}

void ScriptEngine::SetAudioEngine(engine::audio::AudioEngine& audio_engine) {
  if (lua_) {
    BindAudio(*lua_, audio_engine);
  }
}

sol::state& ScriptEngine::LuaState() { return *lua_; }

PrefabFactory& ScriptEngine::GetPrefabFactory() { return *prefab_factory_; }

void ScriptEngine::LoadScript(const std::string& path) {
  ReloadScript(path);

  for (const auto& script : watched_scripts_) {
    if (script.path == path) return;
  }

  std::error_code ec;
  auto last_time = std::filesystem::last_write_time(path, ec);
  if (ec) {
    ENGINE_LOG_WARN("Could not get write time for script '{}': {}", path,
                    ec.message());
    return;
  }
  watched_scripts_.push_back({path, last_time});
}

void ScriptEngine::Update() {
  auto now = std::chrono::steady_clock::now();
  if (now - last_check_time_ < check_interval_) return;
  last_check_time_ = now;

  for (auto it = watched_scripts_.begin(); it != watched_scripts_.end();) {
    std::error_code ec;
    auto current_time = std::filesystem::last_write_time(it->path, ec);

    if (ec) {
      ENGINE_LOG_WARN(
          "Script '{}' no longer accessible, stopping watch. Error: {}",
          it->path, ec.message());
      it = watched_scripts_.erase(it);
      continue;
    }

    if (current_time > it->last_write_time) {
      ENGINE_LOG_INFO("Detected change in script '{}', reloading...", it->path);
      ReloadScript(it->path);
      it->last_write_time = current_time;
    }
    ++it;
  }
}

void ScriptEngine::ReloadScript(const std::string& path) {
  if (!lua_) return;

  auto load_result = lua_->load_file(path);
  if (!load_result.valid()) {
    sol::error err = load_result;
    ENGINE_LOG_ERROR("Syntax error in script '{}': {}", path, err.what());
    return;
  }

  sol::protected_function script_func = load_result;
  auto exec_result = script_func();
  if (!exec_result.valid()) {
    sol::error err = exec_result;
    ENGINE_LOG_ERROR("Runtime error executing script '{}': {}", path,
                     err.what());
  } else {
    ENGINE_LOG_INFO("Successfully loaded script '{}'", path);
  }
}

void ScriptEngine::OnCollision(ecs::EntityId e1, ecs::EntityId e2) {
  if (!lua_) return;

  sol::protected_function on_collision = (*lua_)["OnCollision"];
  if (on_collision.valid()) {
    auto result = on_collision(e1, e2);
    if (!result.valid()) {
      sol::error err = result;
      ENGINE_LOG_ERROR("Lua OnCollision Error: {}", err.what());
    }
  }
}

}  // namespace engine::scripting
