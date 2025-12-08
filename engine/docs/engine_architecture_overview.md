# Engine architecture overview

This document provides a high-level overview of the engine's architecture, module boundaries, and dependency rules. It is intended for developers working on the engine core or building complex extensions.

## 1. High-level design

The engine follows a **modular monolith** architecture centered around a **data-oriented** Entity Component System (ECS).

- **Monolithic:** The engine compiles into a single static library (`engine`), reducing build complexity and runtime overhead.
- **Modular:** Functionality is grouped into logical modules (Render, Audio, Net, ECS) with clear separation of concerns.
- **ECS-Centric:** All game logic is driven by the ECS. The "Engine" primarily provides the runtime environment (Window, Audio Device, Network Socket) and the `Registry` for the game to populate.

### Core philosophy

1.  **Strict lifecycle management:** The `EngineRuntime` class is the single source of truth for the application's lifecycle (Start, Loop, Shutdown).
2.  **Abstraction layers:** Core subsystems (Audio, Render) are defined by abstract interfaces (e.g., `AudioEngine`), allowing for different backends (though `Raylib` is the current default).
3.  **Data-driven logic:** Behavior is defined by Systems processing Components, not by inheritance hierarchies.

---

## 2. System context diagram

The following diagram illustrates how the User's game code interacts with the Engine's primary layers.

```mermaid
graph TD
    subgraph "Engine Boundary"
        Core[Core (EngineRuntime)]
        ECS[ECS Registry]
        
        subgraph Subsystems
            Render[Render (Raylib)]
            Audio[Audio (Raylib)]
            Input
            Net[Net (Asio UDP)]
        end
        
        subgraph "Foundation"
            Math
            Time
            Event
            Util
        end
    end

    UserGame[User Game Code] -->|Configures| Core
    UserGame -->|Defines| Components
    UserGame -->|Registers| Systems
    
    Core -->|Initializes| Subsystems
    Core -->|Drives| ECS
    
    ECS -->|Updates| Components
    
    %% Implicit Dependencies
    Systems -.->|Read/Write| Components
    Systems -.->|Call| Subsystems
```

---

## 3. Module breakdown

The engine is organized into the following top-level modules (found in `include/engine/`):

### A. Core (`include/engine/core`)
-   **Responsibility:** Bootstrapping, Main Loop, and Subsystem Orchestration.
-   **Key classes:**
    -   `EngineRuntime`: The root object. Initializes the window, audio device, and network. It owns the `Registry`.
    -   `EngineRuntimeConfig`: Configuration struct (window title, size, target FPS).
-   **Flow:** Users create the runtime, then call `Pump()` in a loop. `Pump()` handles OS events and delegates to the ECS.

### B. ECS (`include/engine/ecs`)
-   **Responsibility:** Managing Entities, Components, and Systems.
-   **Architecture:** Sparse-Set based for cache-efficient iteration.
-   **Key classes:**
    -   `Registry`: The database of all entities and components.
    -   `System`: Logic containers (functions or classes).
    -   `SparseArray<T>`: The storage for a specific component type.

### C. Subsystems
These modules provide specific functionality. They are typically initialized by `Core` but used by `Systems`.

-   **Render (`include/engine/render`):**
    -   Handles windowing, 2D drawing, and sprites.
    -   Abstraction: `Backend` (Window), `Renderer2D`.
    -   Implementation: Raylib.
-   **Audio (`include/engine/audio`):**
    -   Handles sound playback and music.
    -   Abstraction: `AudioEngine`.
    -   Implementation: Raylib (`RaylibAudioEngine`).
-   **Net (`include/engine/net`):**
    -   Provides UDP networking capabilities.
    -   Key Classes: `Client` (Async worker thread), `UdpSocket`, `PacketBuffer`.
    -   Dependency: `asio`.
-   **Input (`include/engine/input`):**
    -   Polled input state (Keyboard, Mouse, Gamepad).

### D. Foundation
Shared utilities used across all modules.

-   **Math:** Vectors (`Vector2`), Collisions (`Rect`), Transforms.
-   **Time:** Clocks, Frame Timers, `TimeDelta`.
-   **Event:** `EventBus` for decoupled inter-system communication.
-   **Resource:** Asset management (loading/unloading textures/sounds).
-   **Util:** Logging, Configuration helpers.

---

## 4. Module boundaries and dependency rules

To maintain maintainability and compilation speed, strictly adhere to these dependency rules:

### 1. The hierarchy rule
Modules are layered. Higher layers can depend on lower layers, but **never** the reverse.

*   **Layer 1 (Root):** `Core` (Depends on everything to coordinate them).
*   **Layer 2 (Logic):** `User Game Code` (Depends on ECS and Subsystems).
*   **Layer 3 (Features):** `Render`, `Audio`, `Net`, `ECS`. (Should be mostly independent of each other).
*   **Layer 4 (Base):** `Math`, `Time`, `Event`, `Util`. (Used by everyone).

### 2. The "ECS purity" rule
-   The core `ECS` module (`registry.h`, `sparse_array.h`) should **NOT** depend on `Render`, `Audio`, or `Net`.
-   It handles *data*.
-   **Exception:** Systems (which are often part of the "Game" or "Extension" layer) *will* bind ECS components to Subsystems (e.g., a `SpriteRendererSystem` reads `Position` and calls `Render::Draw`).

### 3. The "Interface" rule
-   Where possible, depend on abstract interfaces (e.g., `AudioEngine`) rather than concrete implementations (`RaylibAudioEngine`).
-   This allows swapping backends (e.g., for testing or porting) without rewriting game logic.

---

## 5. Key execution flows

### Initialization Flow
1.  **User main:** defines `EngineRuntimeConfig`.
2.  **EngineRuntime::Create():**
    -   Initializes `WindowBackend` (opens window).
    -   Initializes `AudioEngine` (starts audio device).
    -   Creates `Registry`.
3.  **User main:**
    -   Registers Components (e.g., `Position`, `Sprite`).
    -   Registers Systems (e.g., `MovementSystem`, `RenderSystem`).
4.  **Loop start.**

### Frame loop (The `Pump` cycle)
Executed every frame inside `EngineRuntime::Pump()` or the User's loop:

1.  **Input polling:** Backend polls OS events (keys, mouse, close requests).
2.  **Time step:** Calculate `dt` (Delta Time).
3.  **Logic update (Variable):** `Registry::UpdateSystems(dt)` is called.
    -   Systems iterate over components.
    -   Systems publish events to `EventBus`.
4.  **Rendering:**
    -   User/System calls `BeginFrame()`.
    -   Draw commands are issued.
    -   User/System calls `EndFrame()` -> Buffer Swap.

### Networking flow (Async)
Networking runs slightly differently due to its async nature:
1.  **Worker thread:** `net::Client` runs a background thread waiting on UDP sockets.
2.  **Ingest (main thread):** A "NetworkSystem" polls `Client::TryDequeue()` to get packets received since the last frame.
3.  **Process:** Packets are converted into Game Events or Component updates.
4.  **Send (main thread):** Systems call `Client::Enqueue()` to queue outgoing packets.
5.  **Egress (worker thread):** The worker thread wakes up and sends queued packets.
