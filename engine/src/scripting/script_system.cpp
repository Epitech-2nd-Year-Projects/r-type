#include "engine/scripting/script_system.h"

#include "engine/ecs/registry.h"
#include "engine/util/logging.h"

namespace engine::scripting {

ScriptSystem::ScriptSystem(sol::protected_function update_fn)
    : update_fn_(std::move(update_fn)) {}

void ScriptSystem::Update(ecs::Registry& registry, time::TimeDelta dt) {
  if (!update_fn_.valid()) return;

  auto result = update_fn_(dt.as_seconds(), std::ref(registry));

  if (!result.valid()) {
    sol::error err = result;
    ENGINE_LOG_ERROR("Lua System Error: {}", err.what());
  }
}

}  // namespace engine::scripting
