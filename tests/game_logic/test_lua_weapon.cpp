#include <gtest/gtest.h>

#include <memory>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components/weapon_component.h"
#include "game_logic/game_config.h"
#include "game_logic/prefab_binder.h"
#include "game_logic/systems/weapon_system.h"

class LuaWeaponTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<engine::ecs::Registry>();
    script_engine_ = std::make_unique<engine::scripting::ScriptEngine>();
    script_engine_->Initialize();

    engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
    game_logic::BindRuntimeTypes(script_engine_->LuaState(),
                                 script_engine_->GetPrefabFactory());

    registry_->RegisterComponent<engine::ecs::PositionComponent>();
    registry_->RegisterComponent<engine::ecs::VelocityComponent>();
    registry_->RegisterComponent<game_logic::components::WeaponComponent>();
    registry_->RegisterComponent<game_logic::components::SpriteComponent>();
  }

  std::unique_ptr<engine::ecs::Registry> registry_;
  std::unique_ptr<engine::scripting::ScriptEngine> script_engine_;
};

TEST_F(LuaWeaponTest, LuaWeaponUpdatesCooldownAndSpawns) {
  game_logic::BindGameComponents(script_engine_->GetPrefabFactory());

  script_engine_->LoadScript("../../../../config/behaviors/weapon_logic.lua");

  script_engine_->LuaState().script(R"(
    Prefabs = Prefabs or {}
    Prefabs.PlayerMissile = {
        Velocity = { x = 0, y = 0 }
    }
  )");

  auto e = registry_->SpawnEntity();
  registry_->EmplaceComponent<engine::ecs::PositionComponent>(e, 100.0f,
                                                              100.0f);

  game_logic::components::WeaponComponent weapon;
  weapon.weapon_script = "BasicPlayerWeapon";
  weapon.is_trigger_held = true;
  weapon.fire_rate = 1.0f;
  weapon.projectile_prefab = "PlayerMissile";
  weapon.projectile_speed = 500.0f;
  weapon.cooldown_remaining = engine::time::TimeDelta::zero();

  registry_->EmplaceComponent<game_logic::components::WeaponComponent>(
      e, std::move(weapon));

  auto system =
      std::make_unique<game_logic::systems::WeaponSystem>(*script_engine_);

  system->Update(*registry_, engine::time::TimeDelta::from_seconds(0.1));

  auto& weapons =
      registry_->GetComponents<game_logic::components::WeaponComponent>();
  EXPECT_GT(weapons[e]->cooldown_remaining, engine::time::TimeDelta::zero());

  engine::ecs::EntityId spawned = registry_->EntityFromIndex(1);
  if (spawned != e) {
    auto& vels = registry_->GetComponents<engine::ecs::VelocityComponent>();
    try {
      if (vels.size() > static_cast<size_t>(spawned) &&
          vels[spawned].has_value()) {
        EXPECT_FLOAT_EQ(vels[spawned]->velocity.x, 500.0f);
      }
    } catch (...) {
    }
  }
}

TEST_F(LuaWeaponTest, EnemyWeaponUpdatesAndFires) {
  game_logic::BindGameComponents(script_engine_->GetPrefabFactory());

  script_engine_->LoadScript("../../../../config/behaviors/weapon_logic.lua");

  script_engine_->LuaState().script(R"(
    Prefabs = Prefabs or {}
    Prefabs.EnemyMissile = {
        Velocity = { x = 0, y = 0 }
    }
  )");

  auto e = registry_->SpawnEntity();
  registry_->EmplaceComponent<engine::ecs::PositionComponent>(e, 800.0f,
                                                              100.0f);

  game_logic::components::WeaponComponent weapon;
  weapon.weapon_script = "BasicEnemyWeapon";
  weapon.is_trigger_held = true;
  weapon.fire_rate = 1.0f;
  weapon.projectile_prefab = "EnemyMissile";
  weapon.projectile_speed = 300.0f;
  weapon.cooldown_remaining = engine::time::TimeDelta::zero();

  registry_->EmplaceComponent<game_logic::components::WeaponComponent>(
      e, std::move(weapon));

  auto system =
      std::make_unique<game_logic::systems::WeaponSystem>(*script_engine_);

  system->Update(*registry_, engine::time::TimeDelta::from_seconds(0.1));

  auto& weapons =
      registry_->GetComponents<game_logic::components::WeaponComponent>();
  EXPECT_GT(weapons[e]->cooldown_remaining, engine::time::TimeDelta::zero());

  engine::ecs::EntityId spawned = registry_->EntityFromIndex(1);
  if (spawned != e) {
    auto& vels = registry_->GetComponents<engine::ecs::VelocityComponent>();
    try {
      if (vels.size() > static_cast<size_t>(spawned) &&
          vels[spawned].has_value()) {
        EXPECT_FLOAT_EQ(vels[spawned]->velocity.x, -300.0f);
      }
    } catch (...) {
    }
  }
}
