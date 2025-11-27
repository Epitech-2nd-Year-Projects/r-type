/**
 * @file ecs_complete_tests.cpp
 * @brief Comprehensive ECS system testing suite
 * @version 1.0.0
 *
 * @details
 * Complete test coverage for the ECS system including:
 * - Entity creation and destruction
 * - Component lifecycle (add, remove, access)
 * - Zipper iteration (normal and indexed)
 * - Lambda-based systems (simple, with extra args, priorities)
 * - OOP-style systems (ISystem)
 * - Fixed vs. variable timestep execution
 * - Error handling and edge cases
 */

#include "engine/ecs.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "engine/time.h"

using namespace engine::ecs;
using namespace engine::time;

// ==================== TEST INFRASTRUCTURE ====================

#define GREEN "\033[92m"
#define RED "\033[91m"
#define YELLOW "\033[93m"
#define CYAN "\033[96m"
#define RESET "\033[0m"

int test_count = 0;
int test_passed = 0;
int test_failed = 0;

void print_section(const char* name) {
  std::cout << "\n"
            << CYAN << "╔════════════════════════════════════╗" << RESET
            << std::endl;
  std::cout << CYAN << "║ " << name;
  int padding = 32 - strlen(name);
  for (int i = 0; i < padding; ++i) std::cout << " ";
  std::cout << "║" << RESET << std::endl;
  std::cout << CYAN << "╚════════════════════════════════════╝" << RESET
            << std::endl;
}

void print_test(const char* name) {
  std::cout << YELLOW << "\n[TEST " << ++test_count << "] " << name << RESET
            << std::endl;
}

void assert_true(bool condition, const char* message) {
  if (condition) {
    std::cout << GREEN << "  ✓ " << message << RESET << std::endl;
    test_passed++;
  } else {
    std::cout << RED << "  ✗ FAILED: " << message << RESET << std::endl;
    test_failed++;
  }
}

void assert_equal(int actual, int expected, const char* message) {
  if (actual == expected) {
    std::cout << GREEN << "  ✓ " << message << " (value=" << actual << ")"
              << RESET << std::endl;
    test_passed++;
  } else {
    std::cout << RED << "  ✗ FAILED: " << message << " (expected=" << expected
              << ", got=" << actual << ")" << RESET << std::endl;
    test_failed++;
  }
}

void assert_equal_float(float actual, float expected, float epsilon,
                        const char* message) {
  if (std::abs(actual - expected) < epsilon) {
    std::cout << GREEN << "  ✓ " << message << " (value=" << actual << ")"
              << RESET << std::endl;
    test_passed++;
  } else {
    std::cout << RED << "  ✗ FAILED: " << message << " (expected=" << expected
              << ", got=" << actual << ")" << RESET << std::endl;
    test_failed++;
  }
}

void assert_throws(std::function<void()> func, const char* message) {
  try {
    func();
    std::cout << RED << "  ✗ FAILED: " << message << " (no exception thrown)"
              << RESET << std::endl;
    test_failed++;
  } catch (const std::exception& e) {
    std::cout << GREEN << "  ✓ " << message << " (threw: " << e.what() << ")"
              << RESET << std::endl;
    test_passed++;
  }
}

void print_summary() {
  std::cout << "\n"
            << YELLOW << "════════════════════════════════════" << RESET
            << std::endl;
  std::cout << "Total: " << test_count << " | " << GREEN
            << "Passed: " << test_passed << RESET << " | " << RED
            << "Failed: " << test_failed << RESET << std::endl;
  std::cout << YELLOW << "════════════════════════════════════" << RESET
            << std::endl;
}

// ==================== TEST COMPONENTS ====================

struct Position {
  float x = 0.0f;
  float y = 0.0f;

  Position() = default;
  Position(float x, float y) : x(x), y(y) {}
};

struct Velocity {
  float vx = 0.0f;
  float vy = 0.0f;

  Velocity() = default;
  Velocity(float vx, float vy) : vx(vx), vy(vy) {}
};

struct Health {
  int hp = 100;
  int max_hp = 100;

  Health() = default;
  Health(int hp) : hp(hp), max_hp(hp) {}
};

struct Damage {
  int value = 10;

  Damage() = default;
  Damage(int value) : value(value) {}
};

struct Tag {
  std::string name;

  Tag() = default;
  Tag(const std::string& name) : name(name) {}
};

// ==================== TEST SYSTEMS ====================

class PhysicsSystem : public ISystem {
 public:
  int update_count = 0;
  int fixed_update_count = 0;

  void Update(Registry& registry, TimeDelta dt) override {
    ++update_count;
    auto& positions = registry.GetComponents<Position>();
    auto& velocities = registry.GetComponents<Velocity>();

    for (auto&& [pos, vel] : Zipper(positions, velocities)) {
      pos.value().x += vel.value().vx * dt.as_seconds();
      pos.value().y += vel.value().vy * dt.as_seconds();
    }
  }

  void FixedUpdate(Registry& registry, TimeDelta dt) override {
    ++fixed_update_count;
    auto& velocities = registry.GetComponents<Velocity>();

    for (auto& vel : velocities) {
      if (vel.has_value()) {
        vel.value().vy += 9.81f * dt.as_seconds();
      }
    }
  }
};

class LoggingSystem : public ISystem {
 public:
  int log_count = 0;
  std::vector<std::pair<size_t, Position>> logged_positions;

  void Update(Registry& registry, TimeDelta dt) override {
    ++log_count;
    auto& positions = registry.GetComponents<Position>();
    auto& tags = registry.GetComponents<Tag>();

    for (auto&& [idx, pos, tag] : IndexedZipper(positions, tags)) {
      if (pos.has_value() && tag.has_value()) {
        logged_positions.push_back({idx, pos.value()});
      }
    }
  }
};

// ==================== ENTITY CREATION TESTS ====================

void test_spawn_entity() {
  print_test("Spawn Entity");

  Registry registry;
  EntityId entity1 = registry.SpawnEntity();
  EntityId entity2 = registry.SpawnEntity();

  assert_true(entity1 != entity2, "Two entities have different IDs");
}

void test_spawn_multiple_entities() {
  print_test("Spawn Multiple Entities");

  Registry registry;
  std::vector<EntityId> entities;

  for (int i = 0; i < 10; ++i) {
    entities.push_back(registry.SpawnEntity());
  }

  for (int i = 0; i < 10; ++i) {
    for (int j = i + 1; j < 10; ++j) {
      assert_true(entities[i] != entities[j], "Entity IDs are unique");
    }
  }
}

void test_entity_id_conversion() {
  print_test("EntityId Conversion");

  Registry registry;
  EntityId entity = registry.SpawnEntity();
  size_t idx = static_cast<size_t>(entity);

  assert_true(idx >= 0, "EntityId converts to valid index");
}

// ==================== COMPONENT REGISTRATION TESTS ====================

void test_register_component() {
  print_test("Register Component");

  Registry registry;
  auto& positions = registry.RegisterComponent<Position>();

  assert_true(positions.size() == 0,
              "Empty component array after registration");
}

void test_register_multiple_components() {
  print_test("Register Multiple Components");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();
  registry.RegisterComponent<Health>();

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();
  auto& health = registry.GetComponents<Health>();

  assert_true(positions.size() == 0, "Position array empty");
  assert_true(velocities.size() == 0, "Velocity array empty");
  assert_true(health.size() == 0, "Health array empty");
}

void test_get_unregistered_component_throws() {
  print_test("Get Unregistered Component Throws");

  Registry registry;

  assert_throws([&]() { registry.GetComponents<Position>(); },
                "GetComponents throws for unregistered component");
}

// ==================== COMPONENT ATTACHMENT TESTS ====================

void test_add_component() {
  print_test("Add Component");

  Registry registry;
  registry.RegisterComponent<Position>();

  EntityId entity = registry.SpawnEntity();
  registry.AddComponent<Position>(entity, Position(10.0f, 20.0f));

  auto& positions = registry.GetComponents<Position>();
  assert_true(positions[static_cast<size_t>(entity)].has_value(),
              "Component added to entity");
}

void test_add_multiple_components() {
  print_test("Add Multiple Components");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();
  registry.RegisterComponent<Health>();

  EntityId entity = registry.SpawnEntity();
  registry.AddComponent<Position>(entity, Position(5.0f, 10.0f));
  registry.AddComponent<Velocity>(entity, Velocity(1.0f, 2.0f));
  registry.AddComponent<Health>(entity, Health(100));

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();
  auto& health = registry.GetComponents<Health>();

  assert_true(positions[static_cast<size_t>(entity)].has_value(),
              "Position component added");
  assert_true(velocities[static_cast<size_t>(entity)].has_value(),
              "Velocity component added");
  assert_true(health[static_cast<size_t>(entity)].has_value(),
              "Health component added");
}

void test_emplace_component() {
  print_test("Emplace Component");

  Registry registry;
  registry.RegisterComponent<Position>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 15.0f, 25.0f);

  auto& positions = registry.GetComponents<Position>();
  auto& pos_opt = positions[static_cast<size_t>(entity)];

  assert_true(pos_opt.has_value(), "Component emplaced");
  assert_equal_float(pos_opt.value().x, 15.0f, 0.01f, "Position X correct");
  assert_equal_float(pos_opt.value().y, 25.0f, 0.01f, "Position Y correct");
}

void test_component_access() {
  print_test("Component Access");

  Registry registry;
  registry.RegisterComponent<Position>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 5.0f, 10.0f);

  auto& positions = registry.GetComponents<Position>();
  auto& pos = positions[static_cast<size_t>(entity)];

  assert_equal_float(pos.value().x, 5.0f, 0.01f, "Read X coordinate");
  assert_equal_float(pos.value().y, 10.0f, 0.01f, "Read Y coordinate");

  pos.value().x = 20.0f;
  assert_equal_float(positions[static_cast<size_t>(entity)].value().x, 20.0f,
                     0.01f, "Modify component");
}

void test_remove_component() {
  print_test("Remove Component");

  Registry registry;
  registry.RegisterComponent<Position>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 5.0f, 10.0f);

  auto& positions = registry.GetComponents<Position>();
  assert_true(positions[static_cast<size_t>(entity)].has_value(),
              "Component exists before removal");

  registry.RemoveComponent<Position>(entity);
  assert_true(!positions[static_cast<size_t>(entity)].has_value(),
              "Component removed");
}

void test_kill_entity() {
  print_test("Kill Entity");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 1.0f, 2.0f);
  registry.EmplaceComponent<Velocity>(entity, 3.0f, 4.0f);

  registry.KillEntity(entity);

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();

  assert_true(!positions[static_cast<size_t>(entity)].has_value(),
              "Position removed after KillEntity");
  assert_true(!velocities[static_cast<size_t>(entity)].has_value(),
              "Velocity removed after KillEntity");
}

// ==================== ZIPPER ITERATION TESTS ====================

void test_zipper_basic() {
  print_test("Zipper Basic Iteration");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId e1 = registry.SpawnEntity();
  EntityId e2 = registry.SpawnEntity();
  EntityId e3 = registry.SpawnEntity();

  registry.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
  registry.EmplaceComponent<Velocity>(e1, 10.0f, 20.0f);

  registry.EmplaceComponent<Position>(e2, 3.0f, 4.0f);
  registry.EmplaceComponent<Velocity>(e2, 30.0f, 40.0f);

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();

  int count = 0;
  for (auto&& [pos, vel] : Zipper(positions, velocities)) {
    ++count;
  }

  assert_equal(count, 2, "Zipper iterates over 2 complete pairs");
}

void test_zipper_with_sparse_entities() {
  print_test("Zipper with Sparse Entities");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();
  registry.RegisterComponent<Health>();

  EntityId e1 = registry.SpawnEntity();
  EntityId e2 = registry.SpawnEntity();
  EntityId e3 = registry.SpawnEntity();

  registry.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
  registry.EmplaceComponent<Velocity>(e1, 10.0f, 20.0f);
  registry.EmplaceComponent<Health>(e1, 100);

  registry.EmplaceComponent<Position>(e2, 3.0f, 4.0f);

  registry.EmplaceComponent<Position>(e3, 5.0f, 6.0f);
  registry.EmplaceComponent<Velocity>(e3, 50.0f, 60.0f);
  registry.EmplaceComponent<Health>(e3, 50);

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();
  auto& health = registry.GetComponents<Health>();

  int count = 0;
  for (auto&& [pos, vel, hp] : Zipper(positions, velocities, health)) {
    ++count;
  }

  assert_equal(count, 2, "Zipper skips incomplete entities (e1, e3)");
}

void test_indexed_zipper() {
  print_test("Indexed Zipper");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId e1 = registry.SpawnEntity();
  EntityId e2 = registry.SpawnEntity();

  registry.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
  registry.EmplaceComponent<Velocity>(e1, 10.0f, 20.0f);

  registry.EmplaceComponent<Position>(e2, 3.0f, 4.0f);
  registry.EmplaceComponent<Velocity>(e2, 30.0f, 40.0f);

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();

  std::vector<size_t> indices;
  for (auto&& [idx, pos, vel] : IndexedZipper(positions, velocities)) {
    indices.push_back(idx);
  }

  assert_equal(static_cast<int>(indices.size()), 2,
               "IndexedZipper returns 2 entities");
  assert_true(indices[0] == static_cast<size_t>(e1), "First index matches e1");
  assert_true(indices[1] == static_cast<size_t>(e2), "Second index matches e2");
}

// ==================== LAMBDA-BASED SYSTEM TESTS ====================

void test_add_lambda_system_basic() {
  print_test("Add Lambda System Basic");

  Registry registry;
  registry.RegisterComponent<Position>();

  int call_count = 0;
  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++call_count; },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));
  assert_equal(call_count, 1, "Lambda system called once");
}

void test_add_lambda_system_with_delta() {
  print_test("Add Lambda System With Delta");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 0.0f, 0.0f);
  registry.EmplaceComponent<Velocity>(entity, 10.0f, 0.0f);

  registry.AddSystem<Position, Velocity>(
      [](Registry& reg, SparseArray<Position>& positions,
         SparseArray<Velocity>& velocities, TimeDelta dt) {
        for (auto&& [pos, vel] : Zipper(positions, velocities)) {
          pos.value().x += vel.value().vx * dt.as_seconds();
        }
      },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_seconds(1.0f));

  auto& positions = registry.GetComponents<Position>();
  auto& pos = positions[static_cast<size_t>(entity)];

  assert_equal_float(pos.value().x, 10.0f, 0.01f, "Position updated by system");
}

void test_lambda_system_with_extra_args() {
  print_test("Lambda System With Extra Arguments");

  Registry registry;
  registry.RegisterComponent<Health>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Health>(entity, 100);

  registry.AddSystem<Health>(
      [](Registry& reg, SparseArray<Health>& health, int damage) {
        for (auto& hp : health) {
          if (hp.has_value()) {
            hp.value().hp -= damage;
          }
        }
      },
      SystemType::Variable, kDefaultPriority, 25);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  auto& health = registry.GetComponents<Health>();
  assert_equal(health[static_cast<size_t>(entity)].value().hp, 75,
               "Health reduced by 25");
}

void test_lambda_system_multiple_extra_args() {
  print_test("Lambda System With Multiple Extra Arguments");

  Registry registry;
  registry.RegisterComponent<Health>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Health>(entity, 100);

  registry.AddSystem<Health>(
      [](Registry& reg, SparseArray<Health>& health, int damage,
         float multiplier, bool apply) {
        if (!apply) return;

        for (auto& hp : health) {
          if (hp.has_value()) {
            hp.value().hp -= static_cast<int>(damage * multiplier);
          }
        }
      },
      SystemType::Variable, kDefaultPriority, 10, 2.5f, true);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  auto& health = registry.GetComponents<Health>();
  assert_equal(health[static_cast<size_t>(entity)].value().hp, 75,
               "Health reduced by 10 * 2.5 = 25");
}

// ==================== OOP SYSTEM TESTS ====================

void test_add_oop_system() {
  print_test("Add OOP System (AddSystemClass)");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 0.0f, 0.0f);
  registry.EmplaceComponent<Velocity>(entity, 5.0f, 0.0f);

  auto physics = std::make_shared<PhysicsSystem>();
  registry.AddSystemClass(physics, SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_seconds(1.0f));

  assert_equal(physics->update_count, 1, "OOP system Update called");

  auto& positions = registry.GetComponents<Position>();
  auto& pos = positions[static_cast<size_t>(entity)];

  assert_equal_float(pos.value().x, 5.0f, 0.01f,
                     "Position updated by OOP system");
}

void test_oop_system_fixed_update() {
  print_test("OOP System Fixed Update");

  Registry registry(TimeDelta::from_milliseconds(16.67f));
  registry.RegisterComponent<Velocity>();

  auto physics = std::make_shared<PhysicsSystem>();
  registry.AddSystemClass(physics, SystemType::Fixed);

  registry.UpdateSystems(TimeDelta::from_milliseconds(50.0f));

  assert_true(physics->fixed_update_count >= 2,
              "Fixed update called multiple times");
}

void test_logging_oop_system() {
  print_test("OOP System with Logging");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Tag>();

  EntityId e1 = registry.SpawnEntity();
  EntityId e2 = registry.SpawnEntity();

  registry.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
  registry.EmplaceComponent<Tag>(e1, "Player");

  registry.EmplaceComponent<Position>(e2, 3.0f, 4.0f);
  registry.EmplaceComponent<Tag>(e2, "Enemy");

  auto logger = std::make_shared<LoggingSystem>();
  registry.AddSystemClass(logger, SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  assert_equal(logger->log_count, 1, "Logger called once");
  assert_equal(static_cast<int>(logger->logged_positions.size()), 2,
               "Logged 2 entities");
}

// ==================== SYSTEM PRIORITY TESTS ====================

void test_system_priority_execution_order() {
  print_test("System Priority Execution Order");

  Registry registry;
  registry.RegisterComponent<Position>();

  std::vector<int> execution_order;

  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) {
        execution_order.push_back(1);
      },
      SystemType::Variable, 100);

  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) {
        execution_order.push_back(2);
      },
      SystemType::Variable, 500);

  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) {
        execution_order.push_back(3);
      },
      SystemType::Variable, 0);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  assert_equal(execution_order[0], 2, "High priority system runs first");
  assert_equal(execution_order[1], 1, "Medium priority system runs second");
  assert_equal(execution_order[2], 3, "Low priority system runs last");
}

// ==================== FIXED VS VARIABLE TIMESTEP TESTS ====================

void test_variable_timestep_system() {
  print_test("Variable Timestep System");

  Registry registry;
  registry.RegisterComponent<Position>();

  int call_count = 0;
  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++call_count; },
      SystemType::Variable);

  for (int i = 0; i < 3; ++i) {
    registry.UpdateSystems(TimeDelta::from_milliseconds(16));
  }

  assert_equal(call_count, 3, "Variable system called 3 times");
}

void test_fixed_timestep_system_accumulation() {
  print_test("Fixed Timestep System Accumulation");

  Registry registry(TimeDelta::from_milliseconds(10.0f));
  registry.RegisterComponent<Position>();

  int call_count = 0;
  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++call_count; },
      SystemType::Fixed);

  registry.UpdateSystems(TimeDelta::from_milliseconds(15.0f));
  int first_call_count = call_count;

  registry.UpdateSystems(TimeDelta::from_milliseconds(10.0f));
  int second_call_count = call_count - first_call_count;

  assert_equal(first_call_count, 1, "First frame calls fixed system once");
  assert_equal(second_call_count, 1, "Second frame calls fixed system once");
}

void test_fixed_timestep_multiple_calls() {
  print_test("Fixed Timestep Multiple Calls Per Frame");

  Registry registry(TimeDelta::from_milliseconds(10.0f));
  registry.RegisterComponent<Position>();

  int call_count = 0;
  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++call_count; },
      SystemType::Fixed);

  registry.UpdateSystems(TimeDelta::from_milliseconds(35.0f));

  assert_true(call_count >= 3, "Fixed system called at least 3 times for 35ms");
}

// ==================== MIXED TIMESTEP TESTS ====================

void test_mixed_variable_and_fixed() {
  print_test("Mixed Variable and Fixed Systems");

  Registry registry(TimeDelta::from_milliseconds(10.0f));
  registry.RegisterComponent<Position>();

  int var_count = 0;
  int fixed_count = 0;

  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++var_count; },
      SystemType::Variable);

  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++fixed_count; },
      SystemType::Fixed);

  registry.UpdateSystems(TimeDelta::from_milliseconds(25.0f));

  assert_equal(var_count, 1, "Variable system called once");
  assert_true(fixed_count >= 2, "Fixed system called at least twice");
}

// ==================== SYSTEM MANAGEMENT TESTS ====================

void test_clear_systems() {
  print_test("Clear Systems");

  Registry registry;
  registry.RegisterComponent<Position>();

  int call_count = 0;
  registry.AddSystem<Position>(
      [&](Registry& reg, SparseArray<Position>& positions) { ++call_count; },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));
  assert_equal(call_count, 1, "System called once");

  registry.ClearSystems();
  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  assert_equal(call_count, 1, "System not called after ClearSystems");
}

void test_set_fixed_timestep() {
  print_test("Set Fixed Timestep");

  Registry registry(TimeDelta::from_milliseconds(16.67f));

  TimeDelta initial = registry.FixedTimestep();
  assert_equal_float(initial.as_milliseconds(), 16.67f, 0.1f,
                     "Initial timestep is 16.67ms");

  registry.SetFixedTimestep(TimeDelta::from_milliseconds(33.33f));
  TimeDelta updated = registry.FixedTimestep();

  assert_equal_float(updated.as_milliseconds(), 33.33f, 0.1f,
                     "Updated timestep is 33.33ms");
}

// ==================== EDGE CASES ====================

void test_empty_registry_update() {
  print_test("Empty Registry Update");

  Registry registry;

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));
  assert_true(true, "Empty registry update succeeds");
}

void test_entity_without_components() {
  print_test("Entity Without Components");

  Registry registry;
  registry.RegisterComponent<Position>();

  EntityId entity = registry.SpawnEntity();

  auto& positions = registry.GetComponents<Position>();
  assert_true(!positions[static_cast<size_t>(entity)].has_value(),
              "Entity has no components");
}

void test_system_with_no_matching_entities() {
  print_test("System With No Matching Entities");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId e1 = registry.SpawnEntity();
  EntityId e2 = registry.SpawnEntity();

  registry.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
  registry.EmplaceComponent<Position>(e2, 3.0f, 4.0f);

  int call_count = 0;
  registry.AddSystem<Position, Velocity>(
      [&](Registry& reg, SparseArray<Position>& positions,
          SparseArray<Velocity>& velocities) { ++call_count; },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  assert_equal(call_count, 1, "System called but no entities match");
}

void test_large_entity_count() {
  print_test("Large Entity Count");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  const int ENTITY_COUNT = 1000;

  for (int i = 0; i < ENTITY_COUNT; ++i) {
    EntityId entity = registry.SpawnEntity();
    registry.EmplaceComponent<Position>(entity, static_cast<float>(i), 0.0f);
    registry.EmplaceComponent<Velocity>(entity, 1.0f, 1.0f);
  }

  int processed = 0;
  registry.AddSystem<Position, Velocity>(
      [&](Registry& reg, SparseArray<Position>& positions,
          SparseArray<Velocity>& velocities) {
        for (auto&& [pos, vel] : Zipper(positions, velocities)) {
          ++processed;
        }
      },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  assert_equal(processed, ENTITY_COUNT,
               "All entities processed in large count");
}

// ==================== INTEGRATION TESTS ====================

void test_full_game_loop_simulation() {
  print_test("Full Game Loop Simulation");

  Registry registry(TimeDelta::from_milliseconds(16.67f));
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();
  registry.RegisterComponent<Health>();

  EntityId player = registry.SpawnEntity();
  EntityId enemy = registry.SpawnEntity();

  std::cout << "  DEBUG: player ID = " << static_cast<size_t>(player)
            << ", enemy ID = " << static_cast<size_t>(enemy) << "\n";

  registry.EmplaceComponent<Position>(player, 0.0f, 0.0f);
  registry.EmplaceComponent<Velocity>(player, 5.0f, 0.0f);
  registry.EmplaceComponent<Health>(player, 100);

  registry.EmplaceComponent<Position>(enemy, 100.0f, 0.0f);
  registry.EmplaceComponent<Velocity>(enemy, -5.0f, 0.0f);
  registry.EmplaceComponent<Health>(enemy, 50);

  int system_call_count = 0;
  registry.AddSystem<Position, Velocity>(
      [&system_call_count](Registry& reg, SparseArray<Position>& positions,
                           SparseArray<Velocity>& velocities, TimeDelta dt) {
        int zipper_count = 0;
        for (auto&& [pos, vel] : Zipper(positions, velocities)) {
          ++zipper_count;
          pos.value().x += vel.value().vx * dt.as_seconds();
          pos.value().y += vel.value().vy * dt.as_seconds();
        }
        ++system_call_count;
        if (system_call_count % 10 == 0) {  // Print every 10 frames
          std::cout << "  DEBUG: Frame " << system_call_count
                    << " - Zipper iterated over " << zipper_count
                    << " entities\n";
        }
      },
      SystemType::Fixed);

  registry.AddSystem<Health>(
      [](Registry& reg, SparseArray<Health>& health, float regen_rate) {
        for (auto& hp : health) {
          if (hp.has_value()) {
            hp.value().hp =
                std::min(hp.value().max_hp,
                         hp.value().hp + static_cast<int>(regen_rate));
          }
        }
      },
      SystemType::Variable, kLowPriority, 0.5f);

  for (int frame = 0; frame < 600; ++frame) {
    registry.UpdateSystems(TimeDelta::from_milliseconds(16.67f));
  }

  auto& positions = registry.GetComponents<Position>();
  auto& health = registry.GetComponents<Health>();

  // DEBUG PRINTS BEFORE ASSERTIONS
  std::cout << "  DEBUG: Player position = ("
            << positions[static_cast<size_t>(player)].value().x << ", "
            << positions[static_cast<size_t>(player)].value().y << ")\n";
  std::cout << "  DEBUG: Enemy position = ("
            << positions[static_cast<size_t>(enemy)].value().x << ", "
            << positions[static_cast<size_t>(enemy)].value().y << ")\n";
  std::cout << "  DEBUG: Player health = "
            << health[static_cast<size_t>(player)].value().hp << "\n";

  assert_true(positions[static_cast<size_t>(player)].value().x > 50.0f,
              "Player moved right");
  assert_true(positions[static_cast<size_t>(enemy)].value().x < 50.0f,
              "Enemy moved left");

  assert_equal(health[static_cast<size_t>(player)].value().hp, 100,
               "Player health at max");
}

void test_system_modifying_entities() {
  print_test("System Modifying Entities");

  Registry registry;
  registry.RegisterComponent<Position>();
  registry.RegisterComponent<Velocity>();

  EntityId entity = registry.SpawnEntity();
  registry.EmplaceComponent<Position>(entity, 0.0f, 0.0f);
  registry.EmplaceComponent<Velocity>(entity, 10.0f, 20.0f);

  registry.AddSystem<Position, Velocity>(
      [](Registry& reg, SparseArray<Position>& positions,
         SparseArray<Velocity>& velocities) {
        for (auto&& [pos, vel] : Zipper(positions, velocities)) {
          pos.value().x += 5.0f;
          vel.value().vx *= 2.0f;
        }
      },
      SystemType::Variable);

  registry.UpdateSystems(TimeDelta::from_milliseconds(16));

  auto& positions = registry.GetComponents<Position>();
  auto& velocities = registry.GetComponents<Velocity>();

  assert_equal_float(positions[static_cast<size_t>(entity)].value().x, 5.0f,
                     0.01f, "Position modified by system");
  assert_equal_float(velocities[static_cast<size_t>(entity)].value().vx, 20.0f,
                     0.01f, "Velocity modified by system");
}

// ==================== MAIN ====================

int main() {
  std::cout << "\n"
            << CYAN
            << "╔══════════════════════════════════════════════════════════╗"
            << RESET << std::endl;
  std::cout << CYAN
            << "║         COMPREHENSIVE ECS SYSTEM TEST SUITE              ║"
            << RESET << std::endl;
  std::cout << CYAN
            << "╚══════════════════════════════════════════════════════════╝"
            << RESET << std::endl;

  print_section("ENTITY MANAGEMENT TESTS");
  test_spawn_entity();
  test_spawn_multiple_entities();
  test_entity_id_conversion();

  print_section("COMPONENT REGISTRATION TESTS");
  test_register_component();
  test_register_multiple_components();
  test_get_unregistered_component_throws();

  print_section("COMPONENT ATTACHMENT TESTS");
  test_add_component();
  test_add_multiple_components();
  test_emplace_component();
  test_component_access();
  test_remove_component();
  test_kill_entity();

  print_section("ZIPPER ITERATION TESTS");
  test_zipper_basic();
  test_zipper_with_sparse_entities();
  test_indexed_zipper();

  print_section("LAMBDA-BASED SYSTEM TESTS");
  test_add_lambda_system_basic();
  test_add_lambda_system_with_delta();
  test_lambda_system_with_extra_args();
  test_lambda_system_multiple_extra_args();

  print_section("OOP SYSTEM TESTS (AddSystemClass)");
  test_add_oop_system();
  test_oop_system_fixed_update();
  test_logging_oop_system();

  print_section("SYSTEM PRIORITY TESTS");
  test_system_priority_execution_order();

  print_section("TIMESTEP TESTS");
  test_variable_timestep_system();
  test_fixed_timestep_system_accumulation();
  test_fixed_timestep_multiple_calls();
  test_mixed_variable_and_fixed();

  print_section("SYSTEM MANAGEMENT TESTS");
  test_clear_systems();
  test_set_fixed_timestep();

  print_section("EDGE CASE TESTS");
  test_empty_registry_update();
  test_entity_without_components();
  test_system_with_no_matching_entities();
  test_large_entity_count();

  print_section("INTEGRATION TESTS");
  test_full_game_loop_simulation();
  test_system_modifying_entities();

  print_summary();

  return (test_failed == 0) ? 0 : 1;
}