#include <gtest/gtest.h>

#include <memory>

#include "engine/ecs/registry.h"

struct Vel {
  float x, y;
};

struct Pos {
  float x, y;
};

TEST(RegistryTest, EntityLifecycle) {
  engine::ecs::Registry registry;

  auto e1 = registry.SpawnEntity();
  auto e2 = registry.SpawnEntity();

  EXPECT_NE(e1, e2);

  registry.KillEntity(e1);
}

TEST(RegistryTest, ComponentRegistrationAndAccess) {
  engine::ecs::Registry registry;

  registry.RegisterComponent<Pos>();
  registry.RegisterComponent<Vel>();

  auto e = registry.SpawnEntity();

  registry.EmplaceComponent<Pos>(e, 10.0f, 10.0f);
  registry.AddComponent(e, Vel{1.0f, -1.0f});

  auto& positions = registry.GetComponents<Pos>();
  ASSERT_TRUE(positions[static_cast<std::size_t>(e)].has_value());
  EXPECT_FLOAT_EQ(positions[static_cast<std::size_t>(e)]->x, 10.0f);

  registry.RemoveComponent<Pos>(e);
  EXPECT_FALSE(positions[static_cast<std::size_t>(e)].has_value());
}

TEST(RegistryTest, EntityDestructionCleansComponents) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<Pos>();

  auto e = registry.SpawnEntity();
  registry.EmplaceComponent<Pos>(e, 5.0f, 5.0f);

  auto& positions = registry.GetComponents<Pos>();
  ASSERT_TRUE(positions[static_cast<std::size_t>(e)].has_value());

  registry.KillEntity(e);

  EXPECT_FALSE(positions[static_cast<std::size_t>(e)].has_value());
}

TEST(RegistryTest, SystemExecution) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<Pos>();
  registry.RegisterComponent<Vel>();

  auto e = registry.SpawnEntity();
  registry.EmplaceComponent<Pos>(e, 0.0f, 0.0f);
  registry.EmplaceComponent<Vel>(e, 1.0f, 0.0f);

  bool system_ran = false;

  registry.AddSystem<Pos, Vel>(
      [&system_ran](engine::ecs::Registry& /*reg*/,
                    engine::ecs::SparseArray<Pos>& positions,
                    engine::ecs::SparseArray<Vel>& velocities) {
        system_ran = true;
        for (size_t i = 0; i < positions.size() && i < velocities.size(); ++i) {
          if (positions[i] && velocities[i]) {
            positions[i]->x += velocities[i]->x;
          }
        }
      });

  registry.UpdateSystems(engine::time::TimeDelta::from_seconds(0.1f));

  EXPECT_TRUE(system_ran);
  auto& positions = registry.GetComponents<Pos>();
  EXPECT_FLOAT_EQ(positions[static_cast<std::size_t>(e)]->x, 1.0f);
}
