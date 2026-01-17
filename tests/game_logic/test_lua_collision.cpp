#include <gtest/gtest.h>

#include <memory>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/registry.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/systems/collision_system.h"

using namespace engine::ecs;
using namespace game_logic::components;

class LuaCollisionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry = std::make_unique<Registry>();
    script_engine = std::make_unique<engine::scripting::ScriptEngine>();

    script_engine->Initialize();

    script_engine->LuaState().set_function("SignalPlayerDeath",
                                           [](uint32_t, uint32_t) {});

    engine::scripting::BindRegistry(script_engine->LuaState(), *registry);
    game_logic::BindRuntimeTypes(script_engine->LuaState(),
                                 script_engine->GetPrefabFactory());

    registry->RegisterComponent<TagComponent>();
    registry->RegisterComponent<HealthComponent>();
    registry->RegisterComponent<DamageableComponent>();
    registry->RegisterComponent<game_logic::components::PlayerComponent>();
    registry
        ->RegisterComponent<game_logic::components::DropsPowerupComponent>();
    registry->RegisterComponent<engine::ecs::PositionComponent>();

    script_engine->LoadScript(
        "../../../../config/behaviors/collision_logic.lua");
    script_engine->LoadScript("../../../../config/prefabs/powerups.lua");
  }

  std::unique_ptr<Registry> registry;
  std::unique_ptr<engine::scripting::ScriptEngine> script_engine;
};

TEST_F(LuaCollisionTest, PlayerCrashEnemyReducesHealth) {
  auto player = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(player, "Player");
  registry->EmplaceComponent<HealthComponent>(player, 100, 100);
  registry->EmplaceComponent<game_logic::components::PlayerComponent>(player, 1,
                                                                      0, 0);

  auto enemy = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(enemy, "Enemy");
  registry->EmplaceComponent<HealthComponent>(enemy, 50, 50);

  script_engine->OnCollision(player, enemy);
  auto* p_health =
      registry->GetComponents<HealthComponent>()[player].has_value()
          ? &registry->GetComponents<HealthComponent>()[player].value()
          : nullptr;
  auto* e_health =
      registry->GetComponents<HealthComponent>()[enemy].has_value()
          ? &registry->GetComponents<HealthComponent>()[enemy].value()
          : nullptr;
  auto* p_comp =
      registry->GetComponents<game_logic::components::PlayerComponent>()[player]
              .has_value()
          ? &registry
                 ->GetComponents<
                     game_logic::components::PlayerComponent>()[player]
                 .value()
          : nullptr;

  ASSERT_NE(p_health, nullptr);
  ASSERT_NE(p_comp, nullptr);

  EXPECT_EQ(p_health->current_health, 100);
  EXPECT_EQ(p_comp->lives, 2);

  EXPECT_EQ(e_health, nullptr);
}

TEST_F(LuaCollisionTest, ProjectileHitsTarget) {
  auto enemy = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(enemy, "Enemy");
  registry->EmplaceComponent<HealthComponent>(enemy, 50, 50);

  auto projectile = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(projectile, "Missile");
  registry->EmplaceComponent<DamageableComponent>(projectile, 0, 25, 0);

  script_engine->OnCollision(projectile, enemy);

  auto* e_health =
      registry->GetComponents<HealthComponent>()[enemy].has_value()
          ? &registry->GetComponents<HealthComponent>()[enemy].value()
          : nullptr;
  ASSERT_NE(e_health, nullptr);
  EXPECT_EQ(e_health->current_health, 25);
}

TEST_F(LuaCollisionTest, PlayerRespawnInsteadOfDeath) {
  auto player = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(player, "Player");
  registry->EmplaceComponent<HealthComponent>(player, 100, 10);
  registry->EmplaceComponent<game_logic::components::PlayerComponent>(player, 1,
                                                                      0, 1);
  registry->EmplaceComponent<engine::ecs::PositionComponent>(player, 0.0f,
                                                             0.0f);

  auto enemy = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(enemy, "Enemy");
  registry->EmplaceComponent<HealthComponent>(enemy, 50, 50);

  script_engine->OnCollision(player, enemy);

  auto* p_health =
      registry->GetComponents<HealthComponent>()[player].has_value()
          ? &registry->GetComponents<HealthComponent>()[player].value()
          : nullptr;
  auto* p_comp =
      registry->GetComponents<game_logic::components::PlayerComponent>()[player]
              .has_value()
          ? &registry
                 ->GetComponents<
                     game_logic::components::PlayerComponent>()[player]
                 .value()
          : nullptr;

  ASSERT_NE(p_health, nullptr);
  ASSERT_NE(p_comp, nullptr);

  EXPECT_EQ(p_health->current_health, 100);
  EXPECT_EQ(p_comp->lives, 2);

  auto* p_pos =
      registry->GetComponents<engine::ecs::PositionComponent>()[player]
              .has_value()
          ? &registry->GetComponents<engine::ecs::PositionComponent>()[player]
                 .value()
          : nullptr;
  ASSERT_NE(p_pos, nullptr);
  EXPECT_EQ(p_pos->position.x, 150.0f);
  EXPECT_EQ(p_pos->position.y, 300.0f);
}

TEST_F(LuaCollisionTest, EnemyDropsPowerupOnDeath) {
  auto enemy = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(enemy, "Enemy");
  registry->EmplaceComponent<HealthComponent>(enemy, 50, 10);
  registry->EmplaceComponent<game_logic::components::DropsPowerupComponent>(
      enemy);
  registry->EmplaceComponent<engine::ecs::PositionComponent>(enemy, 100.0f,
                                                             200.0f);

  auto projectile = registry->SpawnEntity();
  registry->EmplaceComponent<TagComponent>(projectile, "Missile");
  registry->EmplaceComponent<DamageableComponent>(projectile, 0, 25, 0);

  script_engine->OnCollision(projectile, enemy);

  bool powerup_found = false;
  auto& positions = registry->GetComponents<engine::ecs::PositionComponent>();
  for (size_t i = 0; i < positions.size(); ++i) {
    if (positions[i].has_value()) {
      auto pos = positions[i]->position;
      if (i != enemy && i != projectile && pos.x == 100.0f && pos.y == 200.0f) {
        powerup_found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(powerup_found);
}
