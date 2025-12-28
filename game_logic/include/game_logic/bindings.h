#ifndef GAME_LOGIC_BINDINGS_H_
#define GAME_LOGIC_BINDINGS_H_

#include <sol/sol.hpp>

namespace game_logic {

/**
 * @brief Bind game-specific runtime types to Lua (Components, Systems helpers)
 * @param lua The Lua state to bind to
 */
void BindRuntimeTypes(sol::state& lua);

}  // namespace game_logic

#endif  // GAME_LOGIC_BINDINGS_H_
