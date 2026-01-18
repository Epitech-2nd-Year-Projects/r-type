# R-Type

## Overview

R-Type is a modern, multiplayer implementation of the classic horizontal shoot-'em-up game, built from scratch using C++23. It features a custom game engine, binary UDP networking, and an authoritative server-client architecture.

## Platforms

- **Windows**: Fully supported (tested on Windows 10/11)
- **Linux**: Fully supported (tested on Fedora/Ubuntu)
- **macOS**: Fully supported (tested on macOS Sequoia)

## Getting started

### Installation

To ensure all dependencies and submodules are correctly initialized, please clone the repository recursively:

```bash
GIT_LFS_SKIP_SMUDGE=1 git clone --recursive git@github.com:Epitech-2nd-Year-Projects/r-type.git
cd r-type
```

> **Note**: If you have already cloned the repository without the `--recursive` flag, you can initialize the submodules manually:
> ```bash
> GIT_LFS_SKIP_SMUDGE=1 git submodule update --init --recursive
> ```

### Prerequisites

- **C++ Compiler**: C++23 compliant (GCC 13+, Clang 16+, MSVC).
- **Git LFS**: Required for large asset management.
- **Xmake**: Build system.
  <details>
  <summary>Click to see installation instructions</summary>

  #### Linux / macOS
  ```bash
  curl -fsSL https://xmake.io/shget.text | bash
  ```
  
  #### Windows (Powershell)
  ```powershell
  Invoke-Expression (Invoke-WebRequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
  ```
  </details>

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

## Troubleshooting

### Common Issues

<details>
<summary><strong>Build fails with "xmake: command not found"</strong></summary>

Ensure Xmake is installed and added to your system's PATH. Refer to the [Prerequisites](#prerequisites) section.
</details>

<details>
<summary><strong>"No route to host" error (Network)</strong></summary>

If you are unable to connect to the server:
1. Check your firewall settings.
2. Verify that both Client and Server are on the same network subnet.
3. Use the correct IP address when launching the client.
</details>

<details>
<summary><strong>Missing or corrupted dependencies</strong></summary>

If you encounter issues related to missing headers or libraries:
1. Delete the `xmake-requires.lock` file.
2. Run `xmake repo -u` to update the package repository.
3. Re-run `xmake` to rebuild.
</details>

<details>
<summary><strong>"raylibmedia.h" header not found</strong></summary>

If compilation fails with `#include <raylibmedia.h>` header not found:
1. Pull the submodules with LFS smudging skipped:
   ```bash
   GIT_LFS_SKIP_SMUDGE=1 git submodule update --init --recursive
   ```
</details>

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

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.