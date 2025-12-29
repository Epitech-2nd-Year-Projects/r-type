#include <gtest/gtest.h>
#include "engine/ecs/registry.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/bindings.h"
#include "game_logic/components.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/player_component.h"
#include "engine/ecs/components/tag_component.h"

class LuaCollisionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    script_engine_ = std::make_unique<engine::scripting::ScriptEngine>();
    script_engine_->Initialize();
    registry_ = std::make_unique<engine::ecs::Registry>();

    engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
    game_logic::BindRuntimeTypes(script_engine_->LuaState(), script_engine_->GetPrefabFactory());
    
    // Bind mock NotifyPlayerDeath
    script_engine_->LuaState().set_function("NotifyPlayerDeath", 
        [this](std::uint32_t id, std::uint8_t lives) {
            last_death_id_ = id;
            last_death_lives_ = lives;
        });

    // Load game_events.lua manually since we don't have GameInstance/Config
    // Assuming relative path from test runner... logic usually loads from absolute path or config
    // Use absolute path for test stability
    script_engine_->LoadScript("/Users/enzogallini/delivery/Tek3/R-TYPE/project/config/behaviors/game_events.lua");
    
#include "game_logic/components/score_value_component.h"
#include "game_logic/components/powerup_drop_component.h"
#include "engine/ecs/components/position_component.h"

// ... (existing includes)

    registry_->RegisterComponent<game_logic::components::HealthComponent>();
    registry_->RegisterComponent<game_logic::components::DamageableComponent>();
    registry_->RegisterComponent<game_logic::components::PlayerComponent>();
    registry_->RegisterComponent<engine::ecs::TagComponent>();
    registry_->RegisterComponent<engine::ecs::PositionComponent>();
    registry_->RegisterComponent<game_logic::components::ScoreValueComponent>();
    registry_->RegisterComponent<game_logic::components::DropsPowerupComponent>();
  }

  std::unique_ptr<engine::scripting::ScriptEngine> script_engine_;
  std::unique_ptr<engine::ecs::Registry> registry_;
  
  std::uint32_t last_death_id_ = 0;
  std::uint8_t last_death_lives_ = 0;
};

TEST_F(LuaCollisionTest, CollisionDamageAndDeath) {
    sol::state& lua = script_engine_->LuaState();
    sol::table game_events = lua["GameEvents"];
    ASSERT_TRUE(game_events.valid());
    sol::function handle_collision = game_events["HandleCollision"];
    ASSERT_TRUE(handle_collision.valid());

    // 1. Create Enemy
    auto enemy = registry_->SpawnEntity();
    registry_->EmplaceComponent<engine::ecs::TagComponent>(enemy, "Enemy");
    registry_->EmplaceComponent<game_logic::components::HealthComponent>(enemy, 100);

    // 2. Create Projectile (Player Faction)
    auto projectile = registry_->SpawnEntity();
    registry_->EmplaceComponent<game_logic::components::DamageableComponent>(projectile, 0, 50); // owner=0, damage=50
    // Fix: DamageableComponent constructor(owner, damage) is (uint32, uint32).
    // Constructor(owner, damage, faction) is (uint32, uint32, uint8).
    // We want faction=0.
    // game_logic::components::DamageableComponent has:
    // DamageableComponent(std::uint32_t owner, std::uint32_t dmg) : owner_id(owner), damage(dmg) {}
    // DamageableComponent(std::uint32_t owner, std::uint32_t dmg, std::uint8_t fac) ...
    // Let's use explicit values: owner=0, damage=50, faction=0.
    auto& dmg_comp = registry_->GetComponents<game_logic::components::DamageableComponent>()[projectile];
    dmg_comp->faction = 0; // Explicitly set if constructor signature is tricky, but Emplace handles args.
    // Actually, explicit constructor likely: EmplaceComponent<T>(entity, 0, 50, 0);

    // 3. Simulate Collision
    handle_collision(projectile, enemy);

    // 4. Verify Enemy took damage
    auto& healths = registry_->GetComponents<game_logic::components::HealthComponent>();
    ASSERT_TRUE(static_cast<size_t>(enemy) < healths.size() && healths[enemy].has_value());
    EXPECT_EQ(healths[enemy]->current_health, 50);

    // 5. Verify Projectile is killed
    // Registry::KillEntity doesn't remove component immediately in some ECS? 
    // Assuming R-Type custom ECS removes it from SparseArray or marks it.
    // If implementation is deferred, we can't check immediately.
    // But `KillEntity` in `registry.h` calls `component_deleters_`.
    // Deleter calls `components.Erase`.
    // `SparseArray::Erase` sets optional to nullopt.
    // So checking `has_value()` should return false.
    auto& damageables = registry_->GetComponents<game_logic::components::DamageableComponent>();
    EXPECT_FALSE(static_cast<size_t>(projectile) < damageables.size() && damageables[projectile].has_value());

    // 6. Projectile hits again to kill
    auto projectile2 = registry_->SpawnEntity();
    registry_->EmplaceComponent<game_logic::components::DamageableComponent>(projectile2, 0, 50, 0);
    
    handle_collision(projectile2, enemy);
    
    EXPECT_EQ(healths[enemy]->current_health, 0);
    EXPECT_FALSE(healths[enemy]->is_alive());

    // 7. Handle Death Logic
    sol::function handle_death = game_events["HandleDeath"];
    ASSERT_TRUE(handle_death.valid());
    
    handle_death(enemy);
    
    // Enemy should be killed
    EXPECT_FALSE(static_cast<size_t>(enemy) < healths.size() && healths[enemy].has_value());
}

TEST_F(LuaCollisionTest, PlayerCrash) {
    sol::state& lua = script_engine_->LuaState();
    sol::table game_events = lua["GameEvents"];
    sol::function handle_collision = game_events["HandleCollision"];

    auto player = registry_->SpawnEntity();
    registry_->EmplaceComponent<engine::ecs::TagComponent>(player, "Player");
    registry_->EmplaceComponent<game_logic::components::HealthComponent>(player, 100);
    registry_->EmplaceComponent<game_logic::components::PlayerComponent>(player);

    auto enemy = registry_->SpawnEntity();
    registry_->EmplaceComponent<engine::ecs::TagComponent>(enemy, "Enemy");
    registry_->EmplaceComponent<game_logic::components::HealthComponent>(enemy, 100);

    // Crash
    handle_collision(player, enemy);

    auto& healths = registry_->GetComponents<game_logic::components::HealthComponent>();
    ASSERT_TRUE(static_cast<size_t>(player) < healths.size() && healths[player].has_value());
    ASSERT_TRUE(static_cast<size_t>(enemy) < healths.size() && healths[enemy].has_value());

    EXPECT_EQ(healths[player]->current_health, 0);
    EXPECT_EQ(healths[enemy]->current_health, 0);
}
