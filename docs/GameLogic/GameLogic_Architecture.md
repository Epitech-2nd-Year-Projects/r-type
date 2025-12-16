# R-Type Game Logic Documentation

**Version:** 1.0  
**Module:** `game_logic`  
**Architecture:** Entity Component System (ECS)

---

## 1. Overview

The `game_logic` module acts as the core library defining the rules, behaviors, and data structures of the R-Type game. It is designed to be **engine-agnostic** regarding rendering but strictly coupled with the custom ECS engine provided in the project.

This library is used by:
- ** The Server:** To maintain the authoritative game state, process collisions, and handle game loops.
- **The Client:** To perform local prediction, handle input-to-gameplay translation, and interpolate states.

### Key Architecture Concepts
The logic follows a strict **ECS (Entity Component System)** pattern:
1.  **Entities:** Unique IDs representing game objects (Players, Enemies, Missiles).
2.  **Components:** Pure data structures attached to entities (e.g., `Position`, `Health`).
3.  **Systems:** Logic processors that iterate over entities possessing specific components to execute behaviors.

---

## 2. Directory Structure

The module is organized to separate data (Components), behavior (Systems), and construction (Entities/Builders).

```text
game_logic/
├── include/game_logic/
│   ├── components/      # Data structures (structs)
│   ├── entities/        # Builders/Factories for creating game objects
│   ├── systems/         # Logic processors (Classes)
│   ├── game_instance.h  # High-level state manager
│   └── game_logic.h     # Main entry point for library initialization
└── src/                 # Implementation files
````

-----

## 3\. Core Components

Components are header-only structs located in `include/game_logic/components/`. They contain **no logic**, only data.

### 3.1. Physics & Transform

| Component | Description |
| :--- | :--- |
| **`PositionComponent`** | Holds X, Y coordinates. |
| **`VelocityComponent`** | Holds vector (dx, dy) for movement calculation. |
| **`SpriteComponent`** | Metadata for rendering (texture ID, scale, source rect). Logic implies *what* to draw, not *how*. |
| **`AnimationComponent`** | Tracks frame state, speed, and loop status for animated sprites. |

### 3.2. Gameplay Data

| Component | Description |
| :--- | :--- |
| **`HealthComponent`** | Current HP and Max HP. |
| **`DamageableComponent`** | Tag/Data indicating an entity can receive damage. |
| **`WeaponComponent`** | Defines fire rate, projectile type, and cooldowns. |
| **`ScoreValueComponent`** | Points awarded to the player upon destroying this entity. |
| **`LifetimeComponent`** | Timer for temporary entities (e.g., explosions, bullets). |

### 3.3. Tags & AI

| Component | Description |
| :--- | :--- |
| **`PlayerComponent`** | Tag identifying a player entity (contains Player ID). |
| **`AIComponent`** | Defines behavior patterns for enemies (e.g., Sine wave movement, tracking). |
| **`PowerupComponent`** | Identifies the type of bonus (Speed, Weapon upgrade). |

-----

## 4\. Systems Architecture

Systems contain the actual code. They inherit from the Engine's `System` class and update the `Registry`.

### 4.1. Core Loop Systems

These systems should run every frame.

  * **`PlayerInputSystem`**: Translates raw input commands into `VelocityComponent` changes or triggers actions (Shoot).
  * **`AISystem`**: Iterates over entities with `AIComponent`. modifies velocity or triggers shooting based on defined patterns (e.g., `Basic`, `Sinusoidal`, `Kamikaze`).
  * **`MovementSystem`**: Applies Velocity to Position: $P_{new} = P_{old} + (V \times \Delta t)$.
  * **`WeaponSystem`**: Manages cooldowns and spawns `Missile` entities when a fire request is active.

### 4.2. Physics & Rules

  * **`CollisionSystem`**: Checks AABB (Axis-Aligned Bounding Box) intersections.
      * *Player vs Enemy*
      * *Bullet vs Enemy*
      * *Player vs Powerup*
      * **Result:** Triggers damage, destroys entities, or applies powerups.
  * **`BoundarySystem`**: Ensures players stay on screen and destroys missiles that leave the game area.

### 4.3. Game State

  * **`HealthSystem`**: Checks if `HealthComponent <= 0`. If true, queues entity for destruction and creates death effects (if applicable).
  * **`WaveSystem`**: Manages the spawning timeline of enemies defined in the Level JSON configuration.

-----

## 5\. Entity Management (Builders)

To avoid code duplication when creating complex entities (like an Enemy which needs Pos, Vel, Sprite, AI, Health, Hitbox...), the project uses the **Builder Pattern**.

Located in `include/game_logic/entities/`.

**Example Workflow (Creating an Enemy):**

```cpp
// Inside WaveSystem or GameLogic
auto enemy = EnemyBuilder()
    .setPosition(100, 200)
    .setHealth(50)
    .setAI(AIType::ZIGZAG)
    .build(registry); // Returns the EntityID
```

**Key Builders:**

  * `PlayerBuilder`: Initializes players with input handling and standard weapons.
  * `MissileBuilder`: Creates projectiles (friendly or hostile).
  * `EnemyBuilder`: Configures different enemy types based on JSON data.
  * `PowerupBuilder`: Spawns bonuses.

-----

## 6\. How to Add New Features

This guide explains how to extend the Game Logic.

### Scenario: Adding a "Shield" Feature

You want a shield that absorbs 1 hit before breaking.

#### Step 1: Create the Component

Create `include/game_logic/components/shield_component.h`.

```cpp
#pragma once

namespace game_logic {
    struct ShieldComponent {
        bool isActive = true;
        // You could add float duration; here if needed
    };
}
```

#### Step 2: Create the System (Optional)

If the shield has active logic (like regenerating or flickering), create a system. If it's passive (checked only during collision), modify an existing system.

Let's modify the **`CollisionSystem`** (`src/systems/collision_system.cpp`) instead of creating a new one, as it changes how damage is received.

```cpp
// In CollisionSystem::run(...)
if (registry.has<ShieldComponent>(entity)) {
    auto& shield = registry.get<ShieldComponent>(entity);
    if (shield.isActive) {
        shield.isActive = false; // Break shield
        // Play sound effect event
        return; // Absorb damage
    }
}
// Proceed to apply damage...
```

#### Step 3: Register Component

In `src/game_logic.cpp` (inside `GameLogic::init` or equivalent), ensure the component is registered with the ECS.

```cpp
registry.registerComponent<ShieldComponent>();
```

#### Step 4: Update Builders

Update `PlayerBuilder` or `PowerupBuilder` to attach this component when appropriate.

```cpp
// In PlayerBuilder.h
entity.addComponent(ShieldComponent{true});
```

-----

## 7\. Configuration & Data-Driven Design

The logic relies heavily on JSON configuration files (located in `config/`).

  * **`enemies.json`**: Defines stats (HP, Speed, Sprite) for enemy types.
  * **`levels/`**: Defines the `WaveSystem` sequence (Time, EnemyType, Position).

**Best Practice:** When adding new enemies or game balance tweaks, modify the JSON files first. Only touch C++ code if new *behavior* (code logic) is required.