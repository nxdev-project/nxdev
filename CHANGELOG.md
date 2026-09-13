# Changelog

All notable changes to the **NXDev** project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.1.0-beta.1] - 2026-09-13

### Highlights
- Initial beta pre-release of the **NXDev** ecosystem for Nintendo Switch homebrew software development.
- Unified native C++20 CLI (`nxdev`) with environment diagnostics, build orchestration, dependency management, artifact packaging, device management, and runtime log streaming.
- Modern C++ platform SDK (`NXDevSDK`) offering RAII wrappers over `libnx` for App lifecycle, Input, RomFS/Filesystem, User Accounts, Sockets/Network, Monotonic Time, and Power.
- Packaging subsystem (`NXDevPack`) supporting Homebrew Menu `.nro` and clean-room `.nsp` output via pinned `hacBrewPack`.
- Visual Studio Code IDE extension (`NXDevToolkit`) with Activity Bar views, status bar integration, diagnostic validation, and WSL support.

### Added
- **NXDevSDK Platform Modules**:
  - `NXDev::Core`: Result codes, `App` lifecycle manager, `scope_exit` guard, `ServiceGuard` RAII, logging sinks, system information queries.
  - `NXDev::Input`: `Controller`, `TouchScreen`, Joy-Con/Pro Controller support, analog sticks with configurable deadzones, button debounce.
  - `NXDev::Filesystem`: `RomFS` mount RAII, safe file reading/writing, SD card paths.
  - `NXDev::Account`: Local user account enumeration, active profile nickname queries.
  - `NXDev::Network`: BSD socket driver lifecycle (`NetworkContext`), local IP resolution.
  - `NXDev::Time`: Monotonic clock, 19.2 MHz AArch64 hardware ticks, calendar time, sleep.
  - `NXDev::Power`: Operation mode (docked vs. handheld), battery percentage and charging state, focus events.
- **NXDevAppManifest**:
  - Declarative specification (`nxapp.yaml`) and JSON Schema (`nxapp.schema.json`).
  - Semantic validator in native C++ (`NXDevAppManifest::Parser`) with precise diagnostic codes and line/column locations.
- **Pluggable Dependency System**:
  - Central package registry (`package-registry/registry.json`) linking logical capabilities to devkitPro pacman portlibs.
  - 3-color topological graph dependency resolver.
  - Commands: `nxdev package <list|info|status|search|install|remove>`.
- **Packaging Backends (`NXDevPack`)**:
  - `.nro` backend: NACP generation via `nacptool`, JPEG icon validation, RomFS embedding, `elf2nro` linking.
  - `.nsp` backend: NPDM compilation (`npdmtool`), ExeFS/NSO staging (`elf2nso`), pinned clean-room `hacBrewPack` backend adapter with user-provided `prod.keys`.
- **Deploy & Run Workflow**:
  - Target device management (`nxdev devices <list|add|remove|set-default|test>`) with project-local and global YAML configurations.
  - Deploy and live log streaming (`nxdev run` and `nxdev deploy`) via `nxlink`.
  - Machine-readable NDJSON streaming (`--json-stream`).
  - Live crash watcher and symbolizer (`nxdev symbolize`) resolving instruction pointers to source code lines via `aarch64-none-elf-addr2line`.
- **Project Scaffolding**:
  - `nxdev new <name>` with starter templates: `minimal-c`, `minimal-cpp`, `console-cpp`, `sdl2`, `deko3d`.
- **VS Code Extension (`NXDevToolkit`)**:
  - Project Explorer, Environment & Diagnostics, SDK Dependencies, and Target Devices tree views.
  - Real-time `nxapp.yaml` semantic diagnostics and quick fixes.
  - Status bar integration and task provider for VS Code build/clean/pack/deploy/run tasks.
  - First-class Windows + WSL2 bridge support.
- **Documentation Suite**:
  - Comprehensive documentation under `docs/` covering Getting Started, Architecture, SDK, Packaging, Run/Deploy, WSL, and Reference.

### Security & Legal
- Zero Nintendo proprietary SDK code, headers, title keys, or cryptographic material.
- Clean-room third-party notices documented in `THIRD_PARTY_NOTICES.md`.
