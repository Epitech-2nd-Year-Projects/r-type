#ifndef GAME_LOGIC_BINDINGS_H_
#define GAME_LOGIC_BINDINGS_H_

#include <sol/sol.hpp>

#include "engine/scripting/prefab_factory.h"

namespace game_logic {

/**
 * @brief Bind game-specific runtime types to Lua (Components, Systems helpers)
 * @param lua The Lua state to bind to
 * @param factory The PrefabFactory for spawning entities
 */
void BindRuntimeTypes(sol::state& lua,
                      engine::scripting::PrefabFactory& factory);

}  // namespace game_logic

#endif  // GAME_LOGIC_BINDINGS_H_
