#ifndef GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_

#include "engine/ecs/component.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"
#include "game_logic/components.h"

namespace game_logic::systems {

/**
 * @class WeaponSystem
 * @brief Handles weapon cooldowns and projectile spawning.
 */
class WeaponSystem : public engine::ecs::ISystem {
 public:
  explicit WeaponSystem(engine::scripting::ScriptEngine &script_engine);
  ~WeaponSystem() override = default;

  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;

 private:
  engine::scripting::ScriptEngine &script_engine_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_WEAPON_SYSTEM_H_
