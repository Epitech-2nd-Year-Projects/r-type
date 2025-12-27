#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/registry.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/game_instance.h"
#include "game_logic/systems/collision_system.h"

using namespace game_logic;

TEST(ScriptingTest, CollisionEvents) {
  auto game = std::make_unique<GameInstance>(1, 4);
  game->Start();

  auto& lua = game->ScriptEngine().LuaState();

  lua["collision_detected"] = false;
  lua["entity_a_id"] = 0;
  lua["entity_b_id"] = 0;

  const std::string script = R"(
    local handle = event_bus:subscribe("OnCollision", function(data)
      collision_detected = true
      entity_a_id = data.entity_a
      entity_b_id = data.entity_b
    end)
  )";

  auto result = lua.script(script);
  EXPECT_TRUE(result.valid());

  auto& registry = game->World();

  auto e1 = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(e1, 100.0f, 100.0f);
  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(e1, 0.0f, 0.0f,
                                                               50.0f, 50.0f);
  registry.EmplaceComponent<engine::ecs::TagComponent>(
      e1, std::string(systems::CollisionSystem::kPlayerTag));

  auto e2 = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(e2, 110.0f, 110.0f);
  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(e2, 0.0f, 0.0f,
                                                               50.0f, 50.0f);
  registry.EmplaceComponent<engine::ecs::TagComponent>(
      e2, std::string(systems::CollisionSystem::kEnemyTag));

  game->Update(engine::time::TimeDelta::from_seconds(1.0f));

  bool detected = lua["collision_detected"];
  engine::ecs::EntityId id_a = lua["entity_a_id"];
  engine::ecs::EntityId id_b = lua["entity_b_id"];

  EXPECT_TRUE(detected);

  bool match = (id_a == e1 && id_b == e2) || (id_a == e2 && id_b == e1);
  EXPECT_TRUE(match);
}
