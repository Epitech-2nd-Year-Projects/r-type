#ifndef ENGINE_SCRIPTING_BINDINGS_H_
#define ENGINE_SCRIPTING_BINDINGS_H_

#include <sol/sol.hpp>
#include <string>

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
 * @brief Bind Engine types (Math, ECS, Components) to the Lua state.
 * @param lua The Lua state to bind to.
 */
void BindTypes(sol::state& lua);

/**
 * @brief Register a specific ECS registry instance as the global 'registry'.
 * @param lua The Lua state.
 * @param registry The active ECS registry instance.
 */
void BindRegistry(sol::state& lua, engine::ecs::Registry& registry);

/**
 * @brief Generic event wrapper for Lua-defined events.
 */
struct LuaEvent {
  std::string name;
  sol::table data;
};

/**
 * @brief Bind the EventBus instance to Lua.
 * @param lua The Lua state.
 * @param event_bus The global event bus.
 */
void BindEventBus(sol::state& lua, engine::event::EventBus& event_bus);

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_BINDINGS_H_
