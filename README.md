# R-Type

## Overview

**R-Type** is a modern, multiplayer implementation of the classic horizontal shoot-'em-up game, built from scratch using
C++23. This project features a robust custom game engine, a binary UDP networking protocol, and an authoritative
server-client architecture designed for performance and extensibility.

The project demonstrates advanced low-level programming concepts including Entity Component System (ECS) architecture,
cross-platform compilation, network programming with Asio, and data-driven game logic.

## Key Features

### 🛠 Custom Game Engine

- **ECS Architecture**: A high-performance Entity Component System managing entities (Players, Enemies, Missiles, etc.)
  and systems (Movement, Collision, Rendering).
- **Rendering Abstraction**: Decoupled rendering layer powered by [Raylib](https://www.raylib.com/), handling sprites,
  animations, and window management.
- **Input System**: normalized input handling supporting keyboard and controllers, mapping raw inputs to semantic game
  actions.
- **Resource Management**: Efficient loading and caching of textures, sounds, and fonts.
- **Audio System**: Abstracted audio playback for music and spatial sound effects.

### 🌐 Networking & Protocol

- **Binary UDP Protocol**: Custom-designed, bandwidth-efficient binary protocol for real-time state synchronization.
- **Authoritative Server**: The server runs the definitive simulation, handling collisions, scoring, and AI, then
  broadcasts world snapshots to clients.
- **Robustness**: Handles packet loss, ordering, timeouts, and client disconnects gracefully.

### 🎮 Gameplay & Logic

- **Data-Driven Design**: Enemies, player stats, and level configurations are defined in external JSON files allowing
  for easy balancing and modding.
    - `config/enemies.json`
    - `config/player.json`
    - `config/levels/`
- **Wave System**: Dynamic enemy spawning patterns (Straight, WavePattern, Patrol, ChasePlayer).
- **Classic Mechanics**: Power-ups, scoring systems, boss battles, and competitive multiplayer support.

## Architecture

The project follows a strict multi-layer architecture to ensure separation of concerns:

- **Engine** (Core): A reusable, component-based framework handling the low-level heavy lifting.
    - **ECS Registry**: Custom sparse-set based entity system.
    - **Asset Manager**: Centralized resource loading.
- **Protocol** (Network): The shared binary specification ensuring deterministic communication.
    - **Header**: Sequence/Ack based reliability header.
    - **Payloads**: Optimized binary structs for events and snapshots.
- **Game Logic** (Rules): The specific implementation of R-Type.
    - **Systems**: Movement, Collision, AI, Waves, Health.
    - **Components**: Data-only structs (Position, Velocity, Health).
- **Server** (Authoritative): Headless application running the simulation.
- **Client** (View): Graphical frontend powered by Raylib.

For a **hyper-detailed** technical deep-dive into the ECS implementation, Packet structure, and System loops, please
read our **[Architecture Documentation](docs/Architecture.md)**.

## Getting Started

### Prerequisites

- **C++ Compiler**: Supporting C++23 (GCC 13+, Clang 16+, MSVC).
- **Xmake**: The build system used for this project.

#### Installing Xmake
- **macOS** (via Homebrew):
  ```bash
  brew install xmake
  ```
- **Linux** (via Shell):
  ```bash
  bash <(curl -fsSL https://xmake.io/shget.text)
  ```
- **Windows** (via Powershell):
  ```powershell
  Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
  ```

### Supported Platforms

- **Linux** (GCC/Clang)
- **macOS** (Clang)
- **Windows** (MSVC)

### Compilation

Use `xmake` to build the project. It will automatically fetch dependencies.

```bash
xmake
```

To build in release mode for optimal performance:

```bash
xmake f -m release
xmake
```

### Usage

#### Running the Server

Launch the dedicated server with defaults:

```bash
xmake run server
```

Use environment variables to tweak settings when needed (e.g.
`RTYPE_SERVER_PORT`, `RTYPE_SERVER_MAX_PLAYERS`, `RTYPE_SERVER_TICK_RATE`,
`RTYPE_SERVER_ROOM_CODE`, `RTYPE_SERVER_LOG_LEVEL`). The server now logs the
room directory periodically, including whether rooms are public or private and
their player counts.

#### Running the Client

Start the client UI:

```bash
xmake run client
```

Configure host, port, nickname, room code, and lobby actions directly from the
main menu. You can refresh the room list, create public or private rooms
(private rooms use a 4-digit code, auto-generated if left blank), and join any
room without CLI flags.

## Configuration

Game balance and entities can be tweaked without recompiling by editing files in the `config/` directory:

- **`enemies.json`**: Define enemy properties (health, speed, behavior, sprite paths).
- **`player.json`**: Configure player speed, lives, and hitboxes.
- **`levels/`**: Level design and wave composition.

## Contributors

| Name               | Roles                     | Contact                                                       |
|--------------------|---------------------------|---------------------------------------------------------------|
| **Enzo Gallini**   | Engine, GameLogic         | [enzo.gallini@epitech.eu](mailto:enzo.gallini@epitech.eu)     |
| **Laurent Aliu**   | Protocol, Server          | [laurent.aliu@epitech.eu](mailto:laurent.aliu@epitech.eu)     |
| **Gregor Sternat** | Server, Client            | [gregor.sternat@epitech.eu](mailto:gregor.sternat@epitech.eu) |
| **Yanis Kernoua**  | Engine, DevOps, GameLogic | [yanis.kernoua@epitech.eu](mailto:yanis.kernoua@epitech.eu)   |
| **Dylan Ta**       | Client, Tests             | [dylan.ta@epitech.eu](mailto:dylan.ta@epitech.eu)             |

---
*Developed as part of the Advanced C++ Knowledge Unit at Epitech.*
