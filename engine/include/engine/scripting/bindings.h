#ifndef ENGINE_SCRIPTING_BINDINGS_H_
#define ENGINE_SCRIPTING_BINDINGS_H_

namespace sol {
class state;
}

namespace engine::ecs {
class Registry;
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

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_BINDINGS_H_
