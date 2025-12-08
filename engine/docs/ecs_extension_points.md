# ECS & extension points

This document outlines the Entity Component System (ECS) architecture, the Event Bus, and how to extend the engine with custom game logic.

## 1. Entity Component System (ECS)

The engine uses a sparse-set based ECS implementation designed for performance and flexibility.

### Core concepts

- **Entity**: A lightweight identifier (`EntityId`). It has no data or behavior itself but groups components together.
- **Component**: Pure data structures (structs/classes) attached to entities.
- **System**: Logic that iterates over entities possessing specific components to perform updates.
- **Registry**: The central manager that handles entities, components, and systems.

### Registry API

The `Registry` class is the main entry point for all ECS operations.

#### Entities

```cpp
#include "engine/ecs/registry.h"

engine::ecs::Registry registry;

// Spawn a new entity
engine::ecs::EntityId player = registry.SpawnEntity();

// Destroy an entity (and all its components)
registry.KillEntity(player);
```

#### Components

Components must be registered before use. They can be any copyable/movable type.

```cpp
struct Position { float x, y; };
struct Velocity { float vx, vy; };

// 1. Register component types
registry.RegisterComponent<Position>();
registry.RegisterComponent<Velocity>();

// 2. Add components to an entity
registry.AddComponent<Position>(player, Position{100.0f, 200.0f});
// Or construct in-place
registry.EmplaceComponent<Velocity>(player, 1.0f, 0.0f);

// 3. Access components (returns SparseArray<T>&)
auto& positions = registry.GetComponents<Position>();

// 4. Remove a component
registry.RemoveComponent<Velocity>(player);
```

#### Systems

Systems are functions or classes that process entities.

**Lambda-based Systems (Preferred):**

```cpp
// Register a system that requires Position and Velocity components
registry.AddSystem<Position, Velocity>(
    [](engine::ecs::Registry& reg, 
       engine::ecs::SparseArray<Position>& positions,
       engine::ecs::SparseArray<Velocity>& velocities,
       engine::time::TimeDelta dt) {
        
        // Iterate over entities that have BOTH Position and Velocity
        for (auto &&[pos, vel] : engine::ecs::Zipper(positions, velocities)) {
            pos.value().x += vel.value().vx * dt.as_seconds();
            pos.value().y += vel.value().vy * dt.as_seconds();
        }
    }
);
```

**System Types:**
- `SystemType::Variable`: Runs every frame (e.g., Rendering, Input).
- `SystemType::Fixed`: Runs at fixed intervals (e.g., Physics).

### Iteration (Zipper)

The `Zipper` helper allows efficient iteration over multiple component arrays simultaneously. It only yields results for entities that possess **all** the requested components.

```cpp
for (auto &&[pos, vel, sprite] : engine::ecs::Zipper(positions, velocities, sprites)) {
    // pos, vel, sprite are std::optional<T>& wrappers
    // Use .value() to access the data
}
```

---

## 2. Event bus

The `EventBus` facilitates decoupled communication between systems.

### Usage

```cpp
#include "engine/event.h"

// 1. Define an event
struct PlayerHitEvent {
    engine::ecs::EntityId entity;
    int damage;
};

engine::event::EventBus event_bus;

// 2. Subscribe
auto handle = event_bus.Subscribe<PlayerHitEvent>(
    [](const PlayerHitEvent& event) {
        // Handle event
    }
);

// 3. Publish (Immediate)
event_bus.Publish(PlayerHitEvent{player_id, 10});

// 4. Enqueue (Deferred)
event_bus.Enqueue<PlayerHitEvent>(player_id, 10);

// ... later in the frame ...
event_bus.DispatchQueued();
```

---

## 3. Extension points: Plugging in custom logic

External games plug into the engine by defining custom components and registering systems.

### Workflow

1.  **Define Components**: Create structs for your game data (Health, Score, AIState, etc.).
2.  **Define Systems**: Write functions that implement your game rules using the components.
3.  **Setup Phase**:
    - Initialize `EngineRuntime`.
    - Register components in the `Registry`.
    - Register systems in the `Registry`.
4.  **Game Loop**:
    - Use `VariableTimestepLoop` or `FixedTimestepLoop`.
    - In the loop callback, call `registry.UpdateSystems(dt)`.

### Example Integration

```cpp
// Components
struct Health { int value; };

// Systems
void DamageSystem(engine::ecs::Registry& reg, 
                  engine::ecs::SparseArray<Health>& healths,
                  engine::time::TimeDelta dt) {
    // Logic...
}

int main() {
    // 1. Initialize Runtime
    engine::core::EngineRuntimeConfig config;
    config.window_config.title = "My Game";
    auto runtime = engine::core::EngineRuntime::Create(config);
    
    // 2. Setup ECS
    auto& registry = runtime->Registry();
    
    registry.RegisterComponent<Health>();
    registry.AddSystem<Health>(DamageSystem);

    // 3. Run Loop
    engine::time::VariableTimestepLoop loop;
    loop.run([&](engine::time::TimeDelta dt) {
        
        // Update Game Logic
        registry.UpdateSystems(dt);
        
        // Render
        runtime->Renderer().BeginFrame();
        // ... draw calls ...
        runtime->Renderer().EndFrame();
        
        return runtime->Pump(); // Return false if window closed
    });

    return 0;
}
```

### Best practices

- **Separation**: Keep rendering logic in `Variable` systems and gameplay/physics in `Fixed` systems.
- **Data-Oriented**: Store data in components, not in systems. Systems should be stateless logic processors where possible.
- **Tags**: Use empty structs as "Tag" components to flag entities (e.g., `IsPlayer`, `IsEnemy`) for easy filtering.
