#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/registry.h"
#include "engine/scripting/script_engine.h"

TEST(ScriptingTest, ScriptSystem_RegistersAndExecutesLuaSystem) {
  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();

  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();

  script_engine.SetRegistry(registry);

  auto& lua = script_engine.LuaState();
  engine::ecs::EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, 0.0f, 0.0f);
  lua["target_entity"] = entity;
  const std::string script = R"(
    function move_system(dt, reg)
       local p = reg:get_position(target_entity)
       if p then
         p.position.x = p.position.x + (10.0 * dt)
         -- Write back (copy semantics)
         reg:add_position(target_entity, p.position.x, p.position.y)
       end
    end

    -- Register as a Variable system (runs every frame)
    registry:register_system("MoveSystem", move_system, SystemType.Variable, 100)
  )";

  auto result = lua.script(script);
  EXPECT_TRUE(result.valid());

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0));

  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  ASSERT_TRUE(positions[entity].has_value());
  EXPECT_FLOAT_EQ(positions[entity].value().position.x, 10.0f);
  EXPECT_FLOAT_EQ(positions[entity].value().position.y, 0.0f);

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(0.5));
  EXPECT_FLOAT_EQ(positions[entity].value().position.x, 15.0f);
}
