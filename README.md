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
| (Modern C++ abstractions: Core, Input, Filesystem, Account, |
|                Network, Time, Power)                        |
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

- **[NXDevSDK](sdk/)**: Modular C++20 platform abstraction library built on top of `libnx`. Provides clean abstractions for application lifecycle (`NXDev::Core`), gamepad/controller input (`NXDev::Input`), RomFS/file I/O (`NXDev::Filesystem`), user accounts (`NXDev::Account`), BSD sockets (`NXDev::Network`), monotonic/wall clocks (`NXDev::Time`), power/event management (`NXDev::Power`), and console/display (`NXDev::Display`).
- **[NXDev Run & Deploy](docs/run/README.md)**: Fast inner development loop for physical Nintendo Switch hardware via `nxlink`, live log streaming, crash detection, and ELF symbolization (`aarch64-none-elf-addr2line`).
- **[NXDevPack](pack/)**: Packaging toolkit providing extensible backend adapters for `.nro` (Homebrew Menu binaries via devkitPro tools) and `.nsp` (standalone installable packages via clean-room hacBrewPack integration).
- **[Package & SDK Subsystem](package-registry/)**: Pluggable dependency management system supporting built-in SDK modules and verified devkitPro portlibs (SDL2, SDL2_image, Deko3D, libcurl, mbedtls, FreeType, Opus, zlib, libpng, libjpeg-turbo, etc.).
- **[NXDev CLI](cli/) (`nxdev`)**: Native, cross-platform unified command-line interface for Switch project compilation ([`nxdev build`](docs/building.md#2-nxdev-build)), application packaging ([`nxdev pack`](docs/pack/README.md)), deployment and live log streaming ([`nxdev run`](docs/run/README.md)), target device management ([`nxdev devices`](docs/run/devices.md)), crash symbolization ([`nxdev symbolize`](docs/run/symbolization.md)), CMake configuration ([`nxdev configure`](docs/building.md#1-nxdev-configure)), package management ([`nxdev package`](docs/packages/devkitpro.md)), environment diagnostics ([`nxdev doctor`](docs/cli.md#1-nxdev-doctor)), environment discovery ([`nxdev env`](docs/cli.md#2-nxdev-env)), project introspection ([`nxdev project`](docs/cli.md#3-nxdev-project)), host configuration ([`nxdev config`](docs/cli.md#4-nxdev-config)), and manifest validation ([`nxdev manifest`](docs/cli.md#5-nxdev-manifest)).
- **[NXDevToolkit](toolkit/vscode/)**: Visual Studio Code extension providing thin IDE integration, task orchestration, device explorer, runtime streaming, and first-class [Windows + WSL workflow support](docs/wsl.md).
- **[NXDevAppManifest](manifest/)**: Declarative application manifest specification (`nxapp.yaml`) and parser library (`NXDevAppManifest::Parser`) for managing application metadata, RomFS directories, NACP/NPDM settings, and build profiles.

---

## Quick CLI Tour

```bash
# Prepare Linux / WSL host with devkitPro prerequisites
nxdev prepare

# Create a new project from template
nxdev new my-game --template console-cpp

# Or initialize an existing codebase
cd my-existing-app
nxdev init

# Verify devkitPro, compilers, and Switch packaging tools
nxdev doctor

# View detected host platform, architecture, and devkitPro paths
nxdev env

# Configure target Switch console
nxdev devices add switch-desk 192.168.1.50 --default

# Compile, pack NRO, deploy, and stream runtime logs in one command:
nxdev --project examples/nro-demo run

# Deploy pre-built NRO to hardware
nxdev --project examples/nro-demo deploy

# Symbolize crash address
nxdev symbolize 0x0000007100014230

# Package application into Home Menu installable (.nsp) package
nxdev --project examples/nsp-demo pack nsp --keys ~/.switch/prod.keys

# Package distributable NXDevSDK archive
nxdev sdk package
```

---

## Documentation

- **[Getting Started Guide](docs/getting-started/README.md)**: End-to-end tutorial from installation to device deployment.
- **[Project Initialization (`nxdev init`)](docs/cli/init.md)**: Adopting and onboarding existing codebases.
- **[Host Preparation (`nxdev prepare`)](docs/cli/prepare.md)**: Linux / WSL devkitPro bootstrap guide.
- **[NXDevSDK Distribution & Packaging](docs/sdk/distribution.md)**: Distributable `NXDevSDK.zip` layout and structure.
- **[Installing NXDevSDK](docs/sdk/installing.md)**: Installing the standalone SDK via `install.sh`.
- **[Run, Deploy & Debug Workflow](docs/run/README.md)**: Hardware deployment, `nxlink` integration, live log streaming, crash analysis, and symbolization.
- **[NXDevPack Subsystem](docs/pack/README.md)**: NRO packaging pipeline, NACP generation, icon validation, and RomFS integration.
- **[Package Subsystem & Dependencies](docs/packages/README.md)**: Pluggable SDK modules, devkitPro portlibs, and registry schema.
- **[NXDevSDK Core & APIs](docs/sdk/README.md)**: Guide to `NXDev::Core`, `Result<T>`, `App`, `log`, and `libnx` interoperability.
- **[Build Pipeline & Workflow](docs/building.md)**: Guide to `nxdev configure`, `nxdev build`, profiles, and outputs.
- **[CMake Integration](docs/cmake.md)**: Toolchain file (`NXDevSwitch.cmake`) and `NXDev::Libnx` target.
- **[CLI Reference](docs/cli.md)**: Full guide to subcommands, global flags, and JSON outputs.
- **[Environment Detection & Precedence](docs/environment.md)**: devkitPro/devkitA64 resolution and tool discovery.
- **[Host Configuration](docs/configuration.md)**: Managing global and project-local developer settings.
- **[WSL Workflow](docs/wsl.md)**: Windows Subsystem for Linux integration and performance guidance.
- **[NXDevToolkit for VS Code](docs/toolkit/README.md)**: Visual Studio Code extension, project explorer, and tasks.
- **[App Manifest Specification](docs/manifest-spec.md)**: Specification for `nxapp.yaml`.
- **[Architecture & Design](docs/architecture.md)**: High-level architectural overview.

---

## Repository Layout

```
nxdev/
├── sdk/                # NXDevSDK core abstractions & platform modules
│   ├── core/           # Base types, Result codes, App lifecycle
│   └── modules/        # Input, Filesystem, Account, Network, Time, Power, Display
├── package-registry/   # Central JSON registry & schema for SDK modules and portlibs
├── cli/                # Native NXDev CLI binary and orchestration engine
├── cmake/              # CMake toolchain, find modules, and target macros
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
