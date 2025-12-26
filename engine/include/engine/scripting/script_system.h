#ifndef ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_
#define ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_

#include <sol/sol.hpp>

#include "engine/ecs/system.h"

namespace engine::scripting {

/**
 * @class ScriptSystem
 * @brief Wrapper for Lua-defined ECS systems.
 *
 * @details
 * Encapsulates a Lua function to be executed as part of the ECS system loop.
 * The Lua function is expected to have the signature:
 * `function(dt: number, registry: Registry)`
 *
 * @note This class is NOT thread-safe for write operations on the Lua state.
 */
class ScriptSystem : public ecs::ISystem {
 public:
  /**
   * @brief Construct a new Script System
   * @param update_fn Lua function to call on update. Signature: function(dt,
   * registry)
   */
  explicit ScriptSystem(sol::protected_function update_fn);

  ~ScriptSystem() override = default;

  void Update(ecs::Registry& registry, time::TimeDelta dt) override;

  /**
   * @brief Update the internal Lua function.
   * @param update_fn New function to replace the existing one.
   */
  void SetFunction(sol::protected_function update_fn);

 private:
  sol::protected_function update_fn_;
};

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_
