#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/event.h"
#include "engine/scripting/script_engine.h"

TEST(ScriptingTest, EventBus_LuaPublishSubscribe) {
  engine::scripting::ScriptEngine script_engine;
  engine::event::EventBus event_bus;
  script_engine.Initialize();
  script_engine.SetEventBus(event_bus);

  auto& lua = script_engine.LuaState();

  const std::string script = R"(
    local received_count = 0
    local received_msg = ""

    function on_event(data)
       received_count = received_count + 1
       received_msg = data.message
    end

    event_bus:subscribe("MyEvent", on_event)

    -- Publish from Lua
    event_bus:publish("MyEvent", {message = "Hello from Lua"})
    
    -- Exposed for C++ verification
    return received_count, received_msg
  )";

  auto result = lua.script(script);
  EXPECT_TRUE(result.valid());

  std::tuple<int, std::string> res = result;
  EXPECT_EQ(std::get<0>(res), 1);
  EXPECT_EQ(std::get<1>(res), "Hello from Lua");
}
