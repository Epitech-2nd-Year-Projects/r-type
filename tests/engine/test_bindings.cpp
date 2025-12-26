#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/registry.h"
#include "engine/scripting/script_engine.h"

TEST(ScriptingTest, BindingsCreateAndModifyEntities) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<engine::ecs::PositionComponent>();
  registry.RegisterComponent<engine::ecs::VelocityComponent>();

  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();
  script_engine.SetRegistry(registry);

  auto& lua = script_engine.LuaState();

  const std::string script = R"(
    local entity = registry:create_entity()
    registry:add_position(entity, 10.0, 20.0)
    registry:add_velocity(entity, 5.0, 0.0)
    
    -- Store entity ID in a global to check it in C++
    last_entity = entity
  )";

  auto result = lua.script(script);
  EXPECT_TRUE(result.valid());

  engine::ecs::EntityId entity = lua["last_entity"];

  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  EXPECT_TRUE(entity < positions.size() && positions[entity].has_value());

  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  EXPECT_TRUE(entity < velocities.size() && velocities[entity].has_value());

  auto& pos = positions[entity].value();
  EXPECT_FLOAT_EQ(pos.position.x, 10.0f);
  EXPECT_FLOAT_EQ(pos.position.y, 20.0f);

  auto& vel = velocities[entity].value();
  EXPECT_FLOAT_EQ(vel.velocity.x, 5.0f);
  EXPECT_FLOAT_EQ(vel.velocity.y, 0.0f);

  const std::string modify_script = R"(
    local p = registry:get_position(last_entity)
    if p then
      p.position.x = 50.0
      registry:add_position(last_entity, p.position.x, p.position.y)
    end
  )";

  auto modify_result = lua.script(modify_script);
  EXPECT_TRUE(modify_result.valid());

  EXPECT_FLOAT_EQ(positions[entity].value().position.x, 50.0f);
}
