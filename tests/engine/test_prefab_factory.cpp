#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/ecs/registry.h"
#include "engine/scripting/prefab_factory.h"

struct TestPos {
  float x, y;
};
struct TestHealth {
  int value;
};

TEST(PrefabFactoryTest, SpawnsEntityWithComponents) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);

  engine::ecs::Registry registry;
  registry.RegisterComponent<TestPos>();
  registry.RegisterComponent<TestHealth>();

  engine::scripting::PrefabFactory factory(lua);

  factory.RegisterComponent("Pos", [](engine::ecs::Registry& r,
                                      engine::ecs::EntityId e,
                                      const sol::object& val) {
    if (val.is<sol::table>()) {
      sol::table t = val.as<sol::table>();
      r.EmplaceComponent<TestPos>(e, t["x"].get_or(0.0f), t["y"].get_or(0.0f));
    }
  });

  factory.RegisterComponent(
      "HP", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
               const sol::object& val) {
        if (val.is<int>()) {
          r.EmplaceComponent<TestHealth>(e, val.as<int>());
        }
      });

  lua.script(R"(
    Prefabs = {
      MyObj = {
        Pos = { x = 12.5, y = 42.0 },
        HP = 100
      }
    }
  )");

  auto e_opt = factory.Spawn(registry, "MyObj");

  ASSERT_TRUE(e_opt.has_value());
  engine::ecs::EntityId e = e_opt.value();

  auto& positions = registry.GetComponents<TestPos>();
  ASSERT_LT(e, positions.size());
  ASSERT_TRUE(positions[e].has_value());
  EXPECT_FLOAT_EQ(positions[e]->x, 12.5f);
  EXPECT_FLOAT_EQ(positions[e]->y, 42.0f);

  auto& healths = registry.GetComponents<TestHealth>();
  ASSERT_LT(e, healths.size());
  ASSERT_TRUE(healths[e].has_value());
  EXPECT_EQ(healths[e]->value, 100);
}

TEST(PrefabFactoryTest, ReturnsNulloptForMissingPrefab) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  engine::ecs::Registry registry;
  engine::scripting::PrefabFactory factory(lua);

  lua.script("Prefabs = {}");

  auto e_opt = factory.Spawn(registry, "NonExistent");
  EXPECT_FALSE(e_opt.has_value());
}

TEST(PrefabFactoryTest, ReturnsNulloptForEmptyName) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  engine::ecs::Registry registry;
  engine::scripting::PrefabFactory factory(lua);

  lua.script("Prefabs = {}");

  auto e_opt = factory.Spawn(registry, "");
  EXPECT_FALSE(e_opt.has_value());
}

TEST(PrefabFactoryTest, ReturnsNulloptForMissingGlobalTable) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  engine::ecs::Registry registry;
  engine::scripting::PrefabFactory factory(lua);

  auto e_opt = factory.Spawn(registry, "Any");
  EXPECT_FALSE(e_opt.has_value());
}

TEST(PrefabFactoryTest, AtomicSpawnRollbackOnException) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  engine::ecs::Registry registry;
  engine::scripting::PrefabFactory factory(lua);

  factory.RegisterComponent(
      "FaultyComp",
      [](engine::ecs::Registry&, engine::ecs::EntityId, const sol::object&) {
        throw std::runtime_error("Component creation failed");
      });

  lua.script(R"(
    Prefabs = {
      BrokenObj = {
        FaultyComp = 1
      }
    }
  )");

  auto e_opt = factory.Spawn(registry, "BrokenObj");
  EXPECT_FALSE(e_opt.has_value());
}

TEST(PrefabFactoryTest, IgnoresUnknownAndNonStringKeys) {
  sol::state lua;
  lua.open_libraries(sol::lib::base);
  engine::ecs::Registry registry;
  engine::scripting::PrefabFactory factory(lua);

  lua.script(R"(
    Prefabs = {
      WeirdObj = {
        [1] = "NonStringKey", 
        UnknownComp = 123
      }
    }
  )");

  auto e_opt = factory.Spawn(registry, "WeirdObj");
  ASSERT_TRUE(e_opt.has_value());
}
