#include <gtest/gtest.h>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/lifetime_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/components/sprite_component.h"
#include "game_logic/game_config.h"
#include "game_logic/prefab_binder.h"

class ObstaclePrefabTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<engine::ecs::Registry>();
    script_engine_ = std::make_unique<engine::scripting::ScriptEngine>();
    script_engine_->Initialize();

    engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
    game_logic::BindGameComponents(script_engine_->GetPrefabFactory());

    registry_->RegisterComponent<engine::ecs::TagComponent>();
    registry_->RegisterComponent<engine::ecs::BoundingBoxComponent>();
    registry_->RegisterComponent<game_logic::components::SpriteComponent>();
    registry_->RegisterComponent<game_logic::components::HealthComponent>();
    registry_->RegisterComponent<game_logic::components::ScoreValueComponent>();
    registry_->RegisterComponent<engine::ecs::PositionComponent>();
    registry_->RegisterComponent<engine::ecs::VelocityComponent>();
    registry_->RegisterComponent<engine::ecs::LifetimeComponent>();
    registry_->RegisterComponent<game_logic::components::DamageableComponent>();

    game_logic::GameConfig::Get().Load("config");
    std::string config_dir = game_logic::GameConfig::Get().GetConfigDirectory();
    script_engine_->LoadScript(config_dir + "/prefabs/obstacles.lua");
  }

  std::unique_ptr<engine::ecs::Registry> registry_;
  std::unique_ptr<engine::scripting::ScriptEngine> script_engine_;
};

TEST_F(ObstaclePrefabTest, SpawnWall) {
  auto opt_entity =
      script_engine_->GetPrefabFactory().Spawn(*registry_, "Wall");
  ASSERT_TRUE(opt_entity.has_value());
  engine::ecs::EntityId entity = *opt_entity;

  auto tags = registry_->GetComponents<engine::ecs::TagComponent>();
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Obstacle");

  auto bboxes = registry_->GetComponents<engine::ecs::BoundingBoxComponent>();
  ASSERT_TRUE(bboxes[entity].has_value());
  EXPECT_FLOAT_EQ(bboxes[entity]->bounds.width_, 64.0f);
  EXPECT_FLOAT_EQ(bboxes[entity]->bounds.height_, 64.0f);

  auto sprites =
      registry_->GetComponents<game_logic::components::SpriteComponent>();
  ASSERT_TRUE(sprites[entity].has_value());
  EXPECT_EQ(sprites[entity]->texture_path, "assets/sprites/obstacle_wall.png");
  EXPECT_EQ(sprites[entity]->layer, 3);

  auto healths =
      registry_->GetComponents<game_logic::components::HealthComponent>();
  EXPECT_FALSE(static_cast<size_t>(entity) < healths.size() &&
               healths[entity].has_value());
}

TEST_F(ObstaclePrefabTest, SpawnDestructibleBarrier) {
  auto opt_entity = script_engine_->GetPrefabFactory().Spawn(
      *registry_, "DestructibleBarrier");
  ASSERT_TRUE(opt_entity.has_value());
  engine::ecs::EntityId entity = *opt_entity;

  auto tags = registry_->GetComponents<engine::ecs::TagComponent>();
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Obstacle");

  auto healths =
      registry_->GetComponents<game_logic::components::HealthComponent>();
  ASSERT_TRUE(healths[entity].has_value());
  EXPECT_EQ(healths[entity]->max_health, 100);

  auto scores =
      registry_->GetComponents<game_logic::components::ScoreValueComponent>();
  ASSERT_TRUE(scores[entity].has_value());
  EXPECT_EQ(scores[entity]->points, 50);
}
