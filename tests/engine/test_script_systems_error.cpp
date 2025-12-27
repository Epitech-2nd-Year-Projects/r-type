#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/ecs/registry.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"

TEST(ScriptingTest, ScriptSystem_HandlesLuaErrorsGracefully) {
  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();

  engine::ecs::Registry registry;
  script_engine.SetRegistry(registry);

  auto& lua = script_engine.LuaState();

  const std::string script = R"(
    function faulty_system(dt, reg)
       error("Simulated Lua Error")
    end
    registry:register_system("FaultySys", faulty_system)
  )";

  auto result = lua.script(script);
  EXPECT_TRUE(result.valid());

  EXPECT_NO_THROW(
      registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0)));
}
