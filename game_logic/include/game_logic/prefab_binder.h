#ifndef GAME_LOGIC_PREFAB_BINDER_H_
#define GAME_LOGIC_PREFAB_BINDER_H_

namespace engine::scripting {
class PrefabFactory;
}

namespace game_logic {

/**
 * @brief Helper to bind game components to the PrefabFactory
 *
 * Registers creators for all components that can be defined in Lua prefabs.
 */
void BindGameComponents(engine::scripting::PrefabFactory& factory);

}  // namespace game_logic

#endif  // GAME_LOGIC_PREFAB_BINDER_H_
