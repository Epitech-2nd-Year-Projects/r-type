#include <gtest/gtest.h>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/lifetime_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components.h"
#include "game_logic/components/powerup_component.h"
#include "game_logic/components/sprite_component.h"
#include "game_logic/game_config.h"
#include "game_logic/prefab_binder.h"

class PowerupPrefabTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<engine::ecs::Registry>();
    script_engine_ = std::make_unique<engine::scripting::ScriptEngine>();
    script_engine_->Initialize();

    engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
    game_logic::BindGameComponents(script_engine_->GetPrefabFactory());
    game_logic::BindRuntimeTypes(script_engine_->LuaState(),
                                 script_engine_->GetPrefabFactory());

    registry_->RegisterComponent<engine::ecs::TagComponent>();
    registry_->RegisterComponent<engine::ecs::VelocityComponent>();
    registry_->RegisterComponent<game_logic::components::PowerupComponent>();
    registry_->RegisterComponent<game_logic::components::SpriteComponent>();
    registry_->RegisterComponent<engine::ecs::PositionComponent>();
    registry_->RegisterComponent<engine::ecs::BoundingBoxComponent>();
    registry_->RegisterComponent<engine::ecs::LifetimeComponent>();

    game_logic::GameConfig::Get().Load("config");
    std::string config_dir = game_logic::GameConfig::Get().GetConfigDirectory();
    script_engine_->LoadScript(config_dir + "/prefabs/powerups.lua");
  }

  std::unique_ptr<engine::ecs::Registry> registry_;
  std::unique_ptr<engine::scripting::ScriptEngine> script_engine_;
};

TEST_F(PowerupPrefabTest, SpawnHealthPotion) {
  auto opt_entity =
      script_engine_->GetPrefabFactory().Spawn(*registry_, "HealthPotion");
  ASSERT_TRUE(opt_entity.has_value());
  engine::ecs::EntityId entity = *opt_entity;

  auto tags = registry_->GetComponents<engine::ecs::TagComponent>();
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Powerup");

  auto vels = registry_->GetComponents<engine::ecs::VelocityComponent>();
  ASSERT_TRUE(vels[entity].has_value());
  EXPECT_FLOAT_EQ(vels[entity]->velocity.x, -50.0f);

  auto powerups =
      registry_->GetComponents<game_logic::components::PowerupComponent>();
  ASSERT_TRUE(powerups[entity].has_value());
  EXPECT_EQ(powerups[entity]->type,
            game_logic::components::PowerupType::kHealth);
  EXPECT_EQ(powerups[entity]->value, 30);
}
