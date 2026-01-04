#include <gtest/gtest.h>

#include <any>
#include <functional>
#include <string>

#include "engine/debug/component_inspector_registry.h"
#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"

namespace {

struct MockHealthComponent {
  int current{100};
  int max{100};
};

struct MockPositionComponent {
  float x{0.0f};
  float y{0.0f};
};

class ComponentInspectorRegistryTest : public ::testing::Test {
 protected:
  engine::debug::ComponentInspectorRegistry registry_;
};

TEST_F(ComponentInspectorRegistryTest, InitiallyEmpty) {
  EXPECT_EQ(registry_.Size(), 0u);
  EXPECT_TRUE(registry_.All().empty());
}

TEST_F(ComponentInspectorRegistryTest, RegisterAndGet) {
  registry_.Register<MockHealthComponent>(
      "Health",
      [](MockHealthComponent& hp, const engine::ecs::EntityId&) -> bool {
        return false;
      });

  EXPECT_EQ(registry_.Size(), 1u);

  auto meta = registry_.Get(std::type_index(typeid(MockHealthComponent)));
  ASSERT_TRUE(meta.has_value());
  EXPECT_EQ(meta->get().display_name, "Health");
}

TEST_F(ComponentInspectorRegistryTest, GetUnregisteredReturnsNullopt) {
  auto meta = registry_.Get(std::type_index(typeid(MockPositionComponent)));
  EXPECT_FALSE(meta.has_value());
}

TEST_F(ComponentInspectorRegistryTest, HasInspector) {
  EXPECT_FALSE(
      registry_.HasInspector(std::type_index(typeid(MockHealthComponent))));

  registry_.Register<MockHealthComponent>(
      "Health",
      [](MockHealthComponent&, const engine::ecs::EntityId&) { return false; });

  EXPECT_TRUE(
      registry_.HasInspector(std::type_index(typeid(MockHealthComponent))));
  EXPECT_FALSE(
      registry_.HasInspector(std::type_index(typeid(MockPositionComponent))));
}

TEST_F(ComponentInspectorRegistryTest, CallbackIsInvoked) {
  bool callback_invoked = false;
  int captured_value = 0;

  registry_.Register<MockHealthComponent>(
      "Health",
      [&callback_invoked, &captured_value](
          MockHealthComponent& hp, const engine::ecs::EntityId&) -> bool {
        callback_invoked = true;
        captured_value = hp.current;
        hp.current = 50;
        return true;
      });

  engine::ecs::Registry registry;
  registry.RegisterComponent<MockHealthComponent>();

  engine::ecs::EntityId entity_id = registry.SpawnEntity();
  registry.AddComponent<MockHealthComponent>(entity_id,
                                             MockHealthComponent{75, 100});

  auto meta = registry_.Get(std::type_index(typeid(MockHealthComponent)));
  ASSERT_TRUE(meta.has_value());

  bool modified = meta->get().draw_fn(registry, entity_id);

  EXPECT_TRUE(callback_invoked);
  EXPECT_EQ(captured_value, 75);

  auto& modified_comp =
      registry.GetComponents<MockHealthComponent>()[static_cast<std::size_t>(
          entity_id)];
  ASSERT_TRUE(modified_comp.has_value());
  EXPECT_EQ(modified_comp->current, 50);
  EXPECT_TRUE(modified);
}

TEST_F(ComponentInspectorRegistryTest, MultipleComponentTypes) {
  registry_.Register<MockHealthComponent>(
      "Health",
      [](MockHealthComponent&, const engine::ecs::EntityId&) { return false; });

  registry_.Register<MockPositionComponent>(
      "Position", [](MockPositionComponent&, const engine::ecs::EntityId&) {
        return false;
      });

  EXPECT_EQ(registry_.Size(), 2u);
  EXPECT_TRUE(
      registry_.HasInspector(std::type_index(typeid(MockHealthComponent))));
  EXPECT_TRUE(
      registry_.HasInspector(std::type_index(typeid(MockPositionComponent))));
}

}  // namespace
