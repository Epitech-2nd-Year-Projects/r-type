#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "engine/scripting/prefab_factory.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/components.h"
#include "game_logic/game_instance.h"
#include "game_logic/prefab_binder.h"

using namespace game_logic;

TEST(PrefabTest, SpawnScout) {
  auto game = std::make_unique<GameInstance>(1, 4);

  auto& script_engine = game->ScriptEngine();
  auto& factory = script_engine.GetPrefabFactory();

  script_engine.LoadScript(
      "/Users/enzogallini/delivery/Tek3/R-TYPE/project/config/prefabs/"
      "enemies.lua");

  auto& registry = game->World();
  auto entity_opt = factory.Spawn(registry, "Scout");

  ASSERT_TRUE(entity_opt.has_value());
  engine::ecs::EntityId entity = *entity_opt;
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  ASSERT_LT(static_cast<size_t>(entity), tags.size());
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Enemy");

  auto& healths = registry.GetComponents<components::HealthComponent>();
  ASSERT_LT(static_cast<size_t>(entity), healths.size());
  ASSERT_TRUE(healths[entity].has_value());
  EXPECT_EQ(healths[entity]->current_health, 10);

  auto& scores = registry.GetComponents<components::ScoreValueComponent>();
  ASSERT_LT(static_cast<size_t>(entity), scores.size());
  ASSERT_TRUE(scores[entity].has_value());
  EXPECT_EQ(scores[entity]->points, 100);

  auto& ais = registry.GetComponents<components::AIComponent>();
  ASSERT_LT(static_cast<size_t>(entity), ais.size());
  ASSERT_TRUE(ais[entity].has_value());
  EXPECT_EQ(ais[entity]->behavior, components::EnemyBehavior::kStraight);
}

TEST(PrefabTest, SpawnBomber) {
  auto game = std::make_unique<GameInstance>(1, 4);
  auto& script_engine = game->ScriptEngine();
  script_engine.LoadScript(
      "/Users/enzogallini/delivery/Tek3/R-TYPE/project/config/prefabs/"
      "enemies.lua");
  auto& factory = script_engine.GetPrefabFactory();
  auto& registry = game->World();

  auto entity_opt = factory.Spawn(registry, "Bomber");
  ASSERT_TRUE(entity_opt.has_value());
  engine::ecs::EntityId entity = *entity_opt;

  auto& ais = registry.GetComponents<components::AIComponent>();
  ASSERT_TRUE(ais[entity].has_value());
  EXPECT_EQ(ais[entity]->behavior, components::EnemyBehavior::kWavePattern);

  auto& healths = registry.GetComponents<components::HealthComponent>();
  ASSERT_TRUE(healths[entity].has_value());
  EXPECT_EQ(healths[entity]->current_health, 20);
}

TEST(PrefabTest, SpawnPlayer) {
  auto game = std::make_unique<GameInstance>(1, 4);
  auto& script_engine = game->ScriptEngine();
  script_engine.LoadScript(
      "/Users/enzogallini/delivery/Tek3/R-TYPE/project/config/prefabs/"
      "players.lua");
  auto& factory = script_engine.GetPrefabFactory();
  auto& registry = game->World();

  auto entity_opt = factory.Spawn(registry, "Player");
  ASSERT_TRUE(entity_opt.has_value());
  engine::ecs::EntityId entity = *entity_opt;

  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Player");

  auto& healths = registry.GetComponents<components::HealthComponent>();
  ASSERT_TRUE(healths[entity].has_value());
  EXPECT_EQ(healths[entity]->current_health, 100);

  auto& players = registry.GetComponents<components::PlayerComponent>();
  ASSERT_TRUE(players[entity].has_value());
  EXPECT_EQ(players[entity]->lives, 3);
  EXPECT_EQ(players[entity]->score, 0);

  auto& weapons = registry.GetComponents<components::WeaponComponent>();
  ASSERT_TRUE(weapons[entity].has_value());
  EXPECT_EQ(weapons[entity]->projectile_prefab, "PlayerMissile");
}

TEST(PrefabTest, SpawnPlayerMissile) {
  auto game = std::make_unique<GameInstance>(1, 4);
  auto& script_engine = game->ScriptEngine();
  script_engine.LoadScript(
      "/Users/enzogallini/delivery/Tek3/R-TYPE/project/config/prefabs/"
      "weapons.lua");
  auto& factory = script_engine.GetPrefabFactory();
  auto& registry = game->World();

  auto entity_opt = factory.Spawn(registry, "PlayerMissile");
  ASSERT_TRUE(entity_opt.has_value());
  engine::ecs::EntityId entity = *entity_opt;

  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  ASSERT_TRUE(tags[entity].has_value());
  EXPECT_EQ(tags[entity]->tag, "Missile");

  auto& damageables = registry.GetComponents<components::DamageableComponent>();
  ASSERT_TRUE(damageables[entity].has_value());
  EXPECT_EQ(damageables[entity]->damage, 10);
  EXPECT_EQ(damageables[entity]->faction, 0);  // Player faction
}
