#include <gtest/gtest.h>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components/ai_component.h"
#include "game_logic/game_config.h"
#include "game_logic/systems/ai_system.h"

class LuaAITest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<engine::ecs::Registry>();
    script_engine_ = std::make_unique<engine::scripting::ScriptEngine>();
    script_engine_->Initialize();

    engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
    game_logic::BindRuntimeTypes(script_engine_->LuaState());

    registry_->RegisterComponent<engine::ecs::PositionComponent>();
    registry_->RegisterComponent<engine::ecs::VelocityComponent>();
    registry_->RegisterComponent<game_logic::components::AIComponent>();

    ai_system_ =
        std::make_unique<game_logic::systems::AISystem>(*script_engine_);
  }

  std::unique_ptr<engine::ecs::Registry> registry_;
  std::unique_ptr<engine::scripting::ScriptEngine> script_engine_;
  std::unique_ptr<game_logic::systems::AISystem> ai_system_;
};

TEST_F(LuaAITest, StraightBehavior) {
  std::string config_dir = game_logic::GameConfig::Get().GetConfigDirectory();

  script_engine_->LoadScript("../../../../config/behaviors/ai.lua");

  sol::table ai_behaviors = script_engine_->LuaState()["AIBehaviors"];
  ASSERT_TRUE(ai_behaviors.valid());

  engine::ecs::EntityId e = registry_->SpawnEntity();
  registry_->EmplaceComponent<engine::ecs::PositionComponent>(e, 100.0f,
                                                              100.0f);
  registry_->EmplaceComponent<engine::ecs::VelocityComponent>(e, 0.0f, 0.0f);

  game_logic::components::AIComponent ai("Straight", 50.0f);
  registry_->EmplaceComponent<game_logic::components::AIComponent>(
      e, std::move(ai));

  ai_system_->Update(*registry_, engine::time::TimeDelta::from_seconds(1.0));

  auto vels = registry_->GetComponents<engine::ecs::VelocityComponent>();
  ASSERT_TRUE(vels[e].has_value());
  EXPECT_FLOAT_EQ(vels[e]->velocity.x, -50.0f);
  EXPECT_FLOAT_EQ(vels[e]->velocity.y, 0.0f);
}

TEST_F(LuaAITest, CustomBehavior) {
  script_engine_->LuaState().script(R"(
    AIBehaviors = AIBehaviors or {}
    function AIBehaviors.TestMove(entity, dt, ai, vel, pos)
      vel.velocity.x = 10.0
      vel.velocity.y = 20.0
    end
  )");

  engine::ecs::EntityId e = registry_->SpawnEntity();
  registry_->EmplaceComponent<engine::ecs::PositionComponent>(e, 0.0f, 0.0f);
  registry_->EmplaceComponent<engine::ecs::VelocityComponent>(e, 0.0f, 0.0f);

  game_logic::components::AIComponent ai("TestMove", 0.0f);
  registry_->EmplaceComponent<game_logic::components::AIComponent>(
      e, std::move(ai));

  ai_system_->Update(*registry_, engine::time::TimeDelta::from_seconds(1.0));

  auto vels = registry_->GetComponents<engine::ecs::VelocityComponent>();
  EXPECT_FLOAT_EQ(vels[e]->velocity.x, 10.0f);
  EXPECT_FLOAT_EQ(vels[e]->velocity.y, 20.0f);
}
