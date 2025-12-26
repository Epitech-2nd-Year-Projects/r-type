#ifndef ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_
#define ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_

#include <sol/sol.hpp>

#include "engine/ecs/system.h"

namespace engine::scripting {

/**
 * @class ScriptSystem
 * @brief Wrapper for Lua-defined ECS systems.
 */
class ScriptSystem : public ecs::ISystem {
 public:
  /**
   * @brief Construct a new Script System
   * @param update_fn Lua function to call on update. Signature: function(dt,
   * registry)
   */
  explicit ScriptSystem(sol::function update_fn);

  ~ScriptSystem() override = default;

  void Update(ecs::Registry& registry, time::TimeDelta dt) override;

 private:
  sol::function update_fn_;
};

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_SCRIPT_SYSTEM_H_
