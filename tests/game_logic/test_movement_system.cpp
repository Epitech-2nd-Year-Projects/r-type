#include <gtest/gtest.h>
#include "engine/ecs/registry.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "game_logic/systems/movement_system.h"
#include "engine/time/time_delta.h"
#include "game_logic/components/player_component.h"

TEST(MovementSystemTest, UpdatesPositionBasedOnVelocity) {
    engine::ecs::Registry registry;
    
    registry.RegisterComponent<engine::ecs::PositionComponent>();
    registry.RegisterComponent<engine::ecs::VelocityComponent>();
    registry.RegisterComponent<game_logic::components::PlayerComponent>();

    engine::ecs::EntityId entity = registry.SpawnEntity();
    
    registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, 0.0f, 0.0f);
    registry.EmplaceComponent<engine::ecs::VelocityComponent>(entity, 10.0f, 0.0f);

    game_logic::systems::MovementSystem movementSystem;

    engine::time::TimeDelta dt = engine::time::TimeDelta::from_seconds(1.0f);
    
    movementSystem.Update(registry, dt);

    auto& pos = registry.GetComponents<engine::ecs::PositionComponent>()[entity];
    ASSERT_TRUE(pos.has_value());
    EXPECT_FLOAT_EQ(pos->position.x, 10.0f);
    EXPECT_FLOAT_EQ(pos->position.y, 0.0f);
}

TEST(MovementSystemTest, UpdatesPositionBasedOnVelocityHalfSecond) {
    engine::ecs::Registry registry;
    registry.RegisterComponent<engine::ecs::PositionComponent>();
    registry.RegisterComponent<engine::ecs::VelocityComponent>();
    registry.RegisterComponent<game_logic::components::PlayerComponent>();

    engine::ecs::EntityId entity = registry.SpawnEntity();
    registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, 10.0f, 10.0f);
    registry.EmplaceComponent<engine::ecs::VelocityComponent>(entity, 0.0f, 20.0f);

    game_logic::systems::MovementSystem movementSystem;
    
    engine::time::TimeDelta dt = engine::time::TimeDelta::from_milliseconds(500);

    movementSystem.Update(registry, dt);

    auto& pos = registry.GetComponents<engine::ecs::PositionComponent>()[entity];
    EXPECT_FLOAT_EQ(pos->position.x, 10.0f);
    EXPECT_FLOAT_EQ(pos->position.y, 20.0f);
}

TEST(MovementSystemTest, NoVelocityNoMovement) {
    engine::ecs::Registry registry;
    registry.RegisterComponent<engine::ecs::PositionComponent>();
    registry.RegisterComponent<engine::ecs::VelocityComponent>();
    registry.RegisterComponent<game_logic::components::PlayerComponent>();

    engine::ecs::EntityId entity = registry.SpawnEntity();
    registry.EmplaceComponent<engine::ecs::PositionComponent>(entity, 5.0f, 5.0f);

    game_logic::systems::MovementSystem movementSystem;
    engine::time::TimeDelta dt = engine::time::TimeDelta::from_seconds(1.0f);

    movementSystem.Update(registry, dt);

    auto& pos = registry.GetComponents<engine::ecs::PositionComponent>()[entity];
    EXPECT_FLOAT_EQ(pos->position.x, 5.0f);
    EXPECT_FLOAT_EQ(pos->position.y, 5.0f);
}
