# R-Type Configuration Guide

**Version:** 1.0  
**System:** `game_config`  
**Format:** JSON

---

## 1. Overview

The R-Type project utilizes a strictly **Data-Driven Architecture**. All gameplay parameters—including enemy statistics, level layouts, physics constants, and asset paths—are externalized in JSON files.

This system allows for:
- **Hot-Reloading (Conceptual):** Tweak values without recompiling.
- **Rapid Prototyping:** Create new levels or enemies solely by editing text files.
- **Modding Support:** Enables external contributions to game content.

### Directory Structure

All configuration files are located in the `config/` directory at the root of the repository.

```text
config/
├── global.json        # World boundaries, spawn rules, and global constants
├── player.json        # Player statistics (speed, health, hitboxes)
├── enemies.json       # Definitions for all enemy types (AI, stats)
├── missiles.json      # Projectile definitions (damage, speed)
├── obstacles.json     # Static environment objects
├── powerups.json      # Bonus items definitions
└── levels/            # Level design sequences
    ├── level_1.json
    └── level_2.json
```

-----

## 2\. Global Configuration (`global.json`)

This file defines the constants that apply to the entire game world, physics engine, and default behaviors.

| Field | Sub-field | Type | Description |
| :--- | :--- | :--- | :--- |
| **world** | `grid_cell_size` | Float | Used for spatial partitioning or debugging grid (in pixels). |
| | `player_spawn_base_x` | Float | The default X coordinate where players spawn. |
| | `player_spawn_offset_x` | Float | Horizontal spacing between players (for multiplayer). |
| | `spawn_min_y` / `max_y` | Float | The vertical range within which entities are allowed to spawn. |
| **enemy\_visuals** | `tint_r/g/b/a` | Int | Default RGBA color multiplier for enemies (0-255). |
| | `layer` | Int | Z-Index for rendering enemies. |
| **collision** | `crash_damage` | Int | Damage taken by the player when colliding directly with an enemy (body slam). |
| **behavior** | `patrol_min/max` | Float | Boundaries (X, Y) for the AI "Patrol" behavior pattern. |

-----

## 3\. Player Configuration (`player.json`)

Defines the base statistics for the player ship.

```json
"default": {
    "health": 100,
    "lives": 3,
    "speed": 200.0,
    ...
}
```

| Field | Type | Unit | Description |
| :--- | :--- | :--- | :--- |
| **health** | Int | HP | Initial health points upon spawning. |
| **lives** | Int | Count | Number of retries before Game Over. |
| **speed** | Float | px/sec | Movement speed of the ship. |
| **hitbox\_width/height** | Float | px | The logical size of the collision box (usually smaller than the sprite). |
| **sprite\_width/height** | Float | px | The rendering size of the texture. |
| **texture\_path** | String | Path | Relative path to the `.png` asset from the binary root. |

-----

## 4\. Enemy Registry (`enemies.json`)

This is a **Key-Value registry** where the key (e.g., "Scout") is the internal ID used in Level files.

### Common Fields

All enemies share these base attributes:

| Field | Type | Description |
| :--- | :--- | :--- |
| **name** | String | Display name (for UI or debugging). |
| **health** | Int | Total HP. |
| **speed** | Float | Movement speed in pixels per second. |
| **behavior** | String | **CRITICAL:** Maps to a C++ AI Strategy. <br>Values: `Straight`, `WavePattern`, `Patrol`, `ChasePlayer`. |
| **score** | Int | Points awarded to the player upon destruction. |
| **texture\_path** | String | Asset path. |

### Behavior-Specific Fields

Certain fields are only required for specific AI behaviors or types:

  * **Shooting Enemies:**
      * `can_shoot` (Bool): Enables the weapon system for this unit.
      * `fire_rate` (Float): Time in seconds between shots.
  * **WavePattern AI:**
      * `wave_amplitude` (Float): Height of the sine wave (pixels).
      * `wave_frequency` (Float): Speed of the oscillation.
  * **ChasePlayer AI:**
      * `detection_range` (Float): Distance (pixels) at which the enemy starts tracking the player.

-----

## 5\. Projectiles (`missiles.json`)

Defines all moving projectiles, whether friendly, hostile, or neutral.

| Field | Type | Description |
| :--- | :--- | :--- |
| **damage** | Int | Amount of Health subtracted on impact. |
| **speed** | Float | Velocity in px/sec. |
| **lifetime** | Float | Time in seconds before the missile automatically despawns (prevents memory leaks). |
| **hitbox\_scale** | Float | Multiplier (0.0 - 1.0) applied to sprite size to calculate collision box. |
| **tint** | Array[4] | RGBA color overlay [R, G, B, A] to recolor sprites programmatically. |

**Notable Types:**

  * `PlayerMissile`: Standard player shot.
  * `EnemyMissile`: Standard enemy shot.
  * `BigPlayerMissile`: Charge shot (higher damage/hitbox).

-----

## 6\. Level Design (`levels/*.json`)

Levels are sequences of "Waves" triggered by a timer.

### Structure

```json
{
    "id": 1,
    "waves": [
        { ... },
        { ... }
    ]
}
```

### Wave Object Definition

Each object in the `waves` array represents a spawn event.

| Field | Type | Mandatory? | Description |
| :--- | :--- | :--- | :--- |
| **time** | Float | **Yes** | The timestamp (in seconds) from the start of the level when this enemy spawns. |
| **type** | String | **Yes** | Must match a Key in `enemies.json` (e.g., "Bomber"). |
| **x** | Float | **Yes** | X spawn coordinate (usually off-screen right, \> 800). |
| **y** | Float | **Yes** | Y spawn coordinate. |
| **random\_y** | Bool | No | If true, overrides `y` with a random value within `global.json` spawn bounds. |
| **drops\_powerup** | Bool | No | If true, this specific enemy will spawn a powerup when destroyed. |

-----

## 7\. Powerups & Obstacles

### `powerups.json`

Defines loot items.

  * `type`: The effect type (currently "Health").
  * `value`: Magnitude of effect (e.g., +30 HP).
  * `drop_probability`: Chance (0.0 to 1.0) to drop if triggered.

### `obstacles.json`

Defines static entities.

  * `destructible` (Bool): Can it be destroyed?
  * `health` (Int): If destructible, how much damage it takes.
  * `score_value` (Int): Points for destroying it (0 for walls).

-----

## 8\. Workflow: How to Add Content

### Example: Adding a "Boss" Enemy

1.  **Asset:** Place `boss.png` in `assets/sprites/`.
2.  **Definition:** Open `config/enemies.json` and add:
    ```json
    "BossMk1": {
        "name": "Mega Boss",
        "health": 5000,
        "speed": 20.0,
        "behavior": "Patrol",
        "score": 10000,
        "sprite_width": 100.0,
        "sprite_height": 100.0,
        "hitbox_width": 100.0,
        "hitbox_height": 100.0,
        "texture_path": "assets/sprites/boss.png",
        "can_shoot": true,
        "fire_rate": 0.2
    }
    ```
3.  **Level:** Open `config/levels/level_2.json` and add a wave entry at the end:
    ```json
    {
        "time": 60.0,
        "type": "BossMk1",
        "x": 900.0,
        "y": 300.0,
        "random_y": false
    }
    ```
4.  **Run:** Launch the game. The Boss will appear at T=60s in Level 2.