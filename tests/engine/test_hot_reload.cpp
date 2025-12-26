#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sol/sol.hpp>
#include <thread>

#include "engine/ecs/registry.h"
#include "engine/scripting/script_engine.h"

namespace fs = std::filesystem;

class HotReloadTest : public ::testing::Test {
 protected:
  void SetUp() override {
    script_path_ = fs::current_path() / "hot_reload_test_system.lua";
    if (fs::exists(script_path_)) fs::remove(script_path_);
  }

  void TearDown() override {
    if (fs::exists(script_path_)) fs::remove(script_path_);
  }

  void WriteScript(const std::string& content) {
    std::ofstream ofs(script_path_);
    ofs << content;
    ofs.close();

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  }

  fs::path script_path_;
};

TEST_F(HotReloadTest, ReloadsSystemLogicOnFly) {
  engine::scripting::ScriptEngine engine;
  engine::ecs::Registry registry;
  engine.Initialize();
  engine.SetRegistry(registry);

  std::string script_v1 = R"(
    Counter = 0
    registry:register_system("TestSystem", function(dt, reg)
        Counter = Counter + 1
    end)
  )";
  WriteScript(script_v1);

  engine.LoadScript(script_path_.string());

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0));

  int counter = engine.LuaState()["Counter"].get_or(0);
  EXPECT_EQ(counter, 1);

  std::string script_v2 = R"(
    -- Note: We don't reset Counter usually, but here we just redefine the system
    registry:register_system("TestSystem", function(dt, reg)
        Counter = Counter + 10
    end)
  )";
  WriteScript(script_v2);

  engine.Update();

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0));

  counter = engine.LuaState()["Counter"].get_or(0);
  EXPECT_EQ(counter, 11);
}

TEST_F(HotReloadTest, IgnoresSyntaxErrorsAndKeepsOldSystem) {
  engine::scripting::ScriptEngine engine;
  engine::ecs::Registry registry;
  engine.Initialize();
  engine.SetRegistry(registry);

  std::string script_v1 = R"(
    Val = 0
    registry:register_system("SolidSystem", function(dt, reg)
        Val = 100
    end)
  )";
  WriteScript(script_v1);
  engine.LoadScript(script_path_.string());

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0));

  {
    int val = engine.LuaState()["Val"].get_or(0);
    EXPECT_EQ(val, 100);
  }

  std::string script_invalid = R"(
    registry:register_system("SolidSystem", function(dt, reg)
        Val = 999
    end -- missing parenthesis or something
    THIS IS GARBAGE SYNTAX
  )";
  WriteScript(script_invalid);
  engine.Update();
  engine.LuaState()["Val"] = 0;
  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(1.0));

  {
    int val = engine.LuaState()["Val"].get_or(0);
    EXPECT_EQ(val, 100);
  }
}
