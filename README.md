# NXDev

[![CI](https://github.com/nxdev-project/nxdev/actions/workflows/ci.yml/badge.svg)](https://github.com/nxdev-project/nxdev/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**NXDev** is an unofficial development ecosystem, SDK abstraction layer, and developer toolkit for Nintendo Switch homebrew software.

> [!IMPORTANT]
> **Disclaimer**: NXDev is an independent, clean-room open-source project and is **not** affiliated with, authorized, endorsed, or supported by Nintendo Co., Ltd. or Nintendo of America Inc. Nintendo Switch is a registered trademark of Nintendo. NXDev contains **no proprietary Nintendo SDK code, headers, binaries, title keys, or cryptographic keys**.

---

## Overview & Philosophy

NXDev is designed to complement and empower the existing Switch homebrew foundation (built by devkitPro, switchbrew, and libnx) by offering modern high-level abstractions, declarative manifests, package management, unified command-line tooling, and editor integration.

```
+-------------------------------------------------------------+
|               Application (Game / Homebrew)                 |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                          NXDevSDK                           |
|       (Modern C++ abstractions: App, Input, Display)        |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                 devkitPro / libnx / Portlibs                |
|             (devkitA64, libnx, switch-tools)                |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                    Horizon OS / Hardware                    |
+-------------------------------------------------------------+
```

---

## Key Components

- **[NXDevSDK](sdk/)**: C++20 abstraction library built on top of `libnx` and devkitPro portlibs. Provides modular interfaces for applet lifecycles, controller inputs, display querying, and graphics without hiding low-level libnx APIs when direct access is needed.
- **[NXDev CLI](cli/) (`nxdev`)**: Native, cross-platform unified command-line interface for scaffolding (`nxdev new`), configuring, building (`nxdev build`), diagnostic checks (`nxdev doctor`), and packaging (`nxdev pack`).
- **[NXDevToolkit](toolkit/vscode/)**: Visual Studio Code extension providing thin IDE integration, task orchestration, and first-class Windows + WSL workflow support.
- **[NXDevPack](pack/)**: Packaging toolkit providing extensible backend adapters for both `.nro` (Homebrew Menu binaries via devkitPro tools) and `.nsp` (standalone installable packages via clean-room hacBrewPack integration).
- **[NXDevAppManifest](manifest/)**: Declarative application manifest specification (`nxapp.yaml`) and parser library (`NXDevAppManifest::Parser`) for managing application metadata, RomFS directories, NACP/NPDM settings, and build profiles.

---

## Current Development Status

> [!NOTE]
> NXDev is currently in **initial foundation / early development stage (`0.1.0-dev`)**.
> The monorepo architecture, build infrastructure, SDK core abstractions, manifest parser, packaging backend interfaces, and CLI skeletons are established. Full devkitA64 cross-compilation toolchain integration, packaging backends, and full manifest schemas are being added progressively.

---

## Repository Layout

```
nxdev/
├── sdk/                # NXDevSDK core abstractions & platform modules
│   ├── core/           # Base types, Result codes, App lifecycle
│   └── modules/        # Input, Display, and future subsystems
├── toolkit/            # Developer tooling & IDE integration
│   └── vscode/         # NXDev Toolkit VS Code extension
├── pack/               # NXDevPack packaging toolkit
│   ├── core/           # PackManager and IPackBackend interfaces
│   ├── nro/            # NRO packaging backend adapter
│   └── nsp/            # NSP packaging backend adapter
├── manifest/           # NXDevAppManifest specification & native parser
│   ├── parser/         # C++20 manifest parser library
│   ├── schema/         # JSON Schema for nxapp.yaml
│   └── docs/           # Manifest documentation
├── cli/                # NXDev unified CLI source (nxdev)
├── cmake/              # CMake helper modules, versioning, warnings
├── templates/          # Project starter templates (basic-cpp)
├── package-registry/   # Portlib and package metadata index
├── third_party/        # Clean third-party dependencies & adapter stubs
├── examples/           # Sample homebrew applications
├── tests/              # Native unit and integration test suite
├── docs/               # Architecture and design specifications
├── scripts/            # Build, test, and formatting automation
├── .github/            # GitHub Actions CI workflows
├── VERSION             # Authoritative version definition
├── LICENSE             # MIT License
└── THIRD_PARTY_NOTICES.md
```

---

## Supported Host Platforms

- **Linux** (Ubuntu 20.04+, Debian, Fedora, Arch Linux)
- **Windows Subsystem for Linux (WSL / WSL2)** *(First-class supported target)*
- **macOS** (Host tools & simulator testing)
- **Native Windows** (Tooling & CLI)

---

## Building from Source

### Prerequisites

- **CMake** 3.20 or newer
- Modern **C++20** compatible compiler (GCC 11+, Clang 13+, or MSVC 2019+)
- *(Optional for Switch binaries)* **devkitPro** & **devkitA64**

### Build Host Components & Run Tests

```bash
# Configure the project
cmake -S . -B build

# Build all native libraries, CLI, examples, and tests
cmake --build build

# Run unit tests
ctest --test-dir build --output-on-failure
```

---

## Contributing & License

Contributions are welcome! Please review [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md) before submitting pull requests.

NXDev is licensed under the [MIT License](LICENSE). Third-party components retain their upstream licenses as outlined in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
