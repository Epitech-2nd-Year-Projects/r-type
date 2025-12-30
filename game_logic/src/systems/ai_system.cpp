#include "game_logic/systems/ai_system.h"

#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/components/ai_component.h"

namespace game_logic::systems {

AISystem::AISystem(engine::scripting::ScriptEngine& script_engine)
    : script_engine_(script_engine) {}

void AISystem::Update(engine::ecs::Registry& registry,
                      engine::time::TimeDelta dt) {
  auto& ais = registry.GetComponents<components::AIComponent>();
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();

  sol::state& lua = script_engine_.LuaState();
  sol::table ai_behaviors = lua["AIBehaviors"];
  if (!ai_behaviors.valid()) {
    return;
  }

  double dt_seconds = dt.as_seconds();

  for (auto [entity_idx, ai, pos, vel] :
       engine::ecs::IndexedZipper(ais, positions, velocities)) {
    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);

    sol::function behavior_func = ai_behaviors[ai->behavior_name];
    if (behavior_func.valid()) {
      behavior_func(entity, dt_seconds, std::ref(*ai), std::ref(*vel),
                    std::ref(*pos));
    } else {
    }
  }
}

}  // namespace game_logic::systems
