# R-Type

## Overview

R-Type is a modern, multiplayer implementation of the classic horizontal shoot-'em-up game, built from scratch using C++23. It features a custom game engine, binary UDP networking, and an authoritative server-client architecture.

## Getting started

### Prerequisites

- **C++ Compiler**: C++23 compliant (GCC 13+, Clang 16+, MSVC).
- **Xmake**: Build system.

**Note:** If Xmake fails to compile, try deleting the `xmake-requires.lock` file to update repository hashes.

### Compilation

Build the project:

```bash
xmake
```

Release mode:

```bash
xmake f -m release
xmake
```

### Usage

**Server:**

```bash
xmake run server
```

**Client:**

```bash
xmake run client
```

## Documentation

For detailed information, please refer to the following documents:

- [Architecture](docs/Architecture.md): Deep dive into ECS, Engine, and System loops.
- [Protocol](docs/protocol_messages.md): Binary UDP protocol specification.
- [Release Workflow](docs/release_workflow.md): Release strategy and tagging system.

## Configuration

Game balance is data-driven via JSON files in the `config/` directory:

- `enemies.json`: Enemy properties.
- `player.json`: Player stats.
- `levels/`: Level definitions.

## Contributors

| Name | Roles | Contact |
|------|-------|---------|
| Enzo Gallini | Engine, GameLogic | enzo.gallini@epitech.eu |
| Laurent Aliu | Protocol, Server | laurent.aliu@epitech.eu |
| Gregor Sternat | Server, Client | gregor.sternat@epitech.eu |
| Yanis Kernoua | Engine, DevOps, GameLogic | yanis.kernoua@epitech.eu |
| Dylan Ta | Client, Tests | dylan.ta@epitech.eu |