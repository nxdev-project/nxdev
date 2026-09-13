# NXDev Architecture Specification

NXDev is an unofficial development ecosystem and SDK for Nintendo Switch homebrew software.

The primary architectural goal of NXDev is to provide modern abstractions, dependency management, project manifests, packaging tools, and IDE integration on top of the established **devkitPro** / **libnx** toolchain without replacing or obfuscating raw underlying capabilities.

---

## 1. Core Architectural Principle

```
+-----------------------------------------------------------+
|              Application (Game / Homebrew)                |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                          NXDevSDK                         |
| (Modern C++ abstractions: Core, Input, Filesystem, Account,|
|                Network, Time, Power)                       |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|               devkitPro / libnx / Portlibs                |
|           (Toolchains, low-level sys-services)            |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                  Horizon OS / Hardware                    |
+-----------------------------------------------------------+
```

NXDev **never** replaces libnx or devkitA64. Instead:
- **devkitPro** remains responsible for the cross-compilation toolchain (`aarch64-none-elf`), system libraries (`libnx`), binary utilities (`switch-tools`), and upstream portlibs.
- **NXDev** provides high-level modular abstractions, manifest specifications (`nxapp.yaml`), unified packaging (`NXDevPack`), declarative build orchestration, CLI workflows (`nxdev`), and VS Code integration (`NXDevToolkit`).

---

## 2. Component Boundaries and Separation of Concerns

```
+-------------------------------------------------------------+
|                 NXDevToolkit (VS Code Ext)                  |
|                 [TypeScript thin IDE layer]                 |
+-------------------------------------------------------------+
                              |
                              | (invokes via CLI / JSON IPC)
                              v
+-------------------------------------------------------------+
|                          NXDev CLI                          |
|         [Native C++20 cross-platform orchestrator]          |
+-------------------------------------------------------------+
         /                     |                     \
        /                      |                      \
       v                       v                       v
+---------------+    +--------------------+    +---------------+
| NXDevManifest |    |     NXDevPack      |    |  SDK / Tools  |
| (nxapp.yaml)  |    |  (NRO/NSP Engines) |    |  (Templates)  |
+---------------+    +--------------------+    +---------------+
                               |
                               v
                     +--------------------+
                     |    devkitPro &     |
                     |  Third-Party Tools |
                     | (elf2nro, hacBP)   |
                     +--------------------+
```

### Component Breakdown

1. **NXDevSDK (`sdk/`)**
   - Implemented in modern C++ (C++20).
   - Provides clean modular abstractions:
     - `NXDev::Core`: Result codes, App lifecycle, ScopeExit, ServiceGuard, logging, system info.
     - `NXDev::Input`: Gamepad, Joy-Con, Pro Controller, analog sticks, dead zones, touch screen.
     - `NXDev::Filesystem`: RomFS mounting RAII, safe file reading/writing, path helpers.
     - `NXDev::Account`: Horizon local user accounts, active user, profile nickname queries.
     - `NXDev::Network`: BSD socket driver RAII lifecycle, local IP queries.
     - `NXDev::Time`: Monotonic clock, hardware AArch64 system ticks (19.2 MHz), calendar time, sleep.
     - `NXDev::Power`: Operation mode (docked/handheld), focus state, battery status, applet event hooks.
   - Does **not** depend on the editor or the CLI.
   - Compiles cleanly on host environments with mocks/stubs and binds directly to native libnx when targeting Nintendo Switch.

2. **NXDevAppManifest (`manifest/`)**
   - Declarative specification standard (`nxapp.yaml`) and JSON Schema.
   - Native C++ parser and validator (`NXDevAppManifest::Parser`).
   - Independent of VS Code, enabling headless CI and CLI usage.

3. **NXDevPack (`pack/`)**
   - Packaging subsystem supporting multiple backends via the `IPackBackend` adapter pattern (`NroPackBackend`, `NspPackBackend`).
   - Does not expose backend-specific internals (e.g. hacBrewPack intricacies) to consumers.
   - Programmatically callable by the CLI or custom build steps.

4. **NXDev CLI (`cli/`)**
   - Unified native C++ executable (`nxdev`).
   - Links directly with `NXDevAppManifest::Parser` and `NXDevPack::Core`.
   - Offers commands: `new`, `doctor`, `build`, `pack`, `version`.
   - Native cross-platform execution on Linux, WSL, macOS, and Windows with zero external runtime overhead.

5. **NXDevToolkit (`toolkit/vscode/`)**
   - TypeScript extension for Visual Studio Code.
   - Acts strictly as a thin frontend over the `nxdev` CLI.
   - Never embeds packaging or build logic inside JavaScript.

---

## 3. Dependency Direction & Anti-Circular Rules

To maintain long-term stability and maintainability:
- `NXDevToolkit` -> depends only on `NXDev CLI`.
- `NXDev CLI` -> depends on `NXDevAppManifest` and `NXDevPack`.
- `NXDevPack` -> depends on `NXDevAppManifest`.
- `NXDevSDK` -> standalone native library; does **not** depend on CLI, Manifest, Pack, or IDE.
- Third-party tools (like `hacBrewPack` or `switch-tools`) -> isolated behind backend adapter interfaces in `NXDevPack`.

---

## 4. Environment & Platform Support

- **Host Platforms**: Linux, Windows Subsystem for Linux (WSL), macOS, Windows.
- **Target Platform**: Nintendo Switch (Horizon OS / devkitA64).
- **Workspace Independence**: No hardcoded paths (`/opt/devkitpro` or home directories). All paths are resolved via environment variables (`DEVKITPRO`), manifest settings, or command-line parameters.

---

## 5. Package & Pluggable SDK Subsystem Architecture

The package management and pluggable SDK subsystem decouples optional SDK capabilities and devkitPro portlibs from the core NXDev codebase:

```
+-----------------------------------------------------------+
|                   Application (nxapp.yaml)                |
|               dependencies: [nxdev.core, nxdev.sdl2]      |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|               DependencyResolver (Topological DFS)        |
+-----------------------------------------------------------+
          /                                       \
         v                                         v
+-------------------------------+       +-------------------------------+
|    Built-in Module Targets    |       | devkitPro Portlibs (pacman)   |
|   (NXDev::Core, NXDev::Input, |       |   (switch-sdl2, deko3d, etc.) |
|    NXDev::Filesystem, etc.)   |       |   Installed to portlibs/switch|
+-------------------------------+       +-------------------------------+
         \                                         /
          v                                       v
+-----------------------------------------------------------+
|          CMake Interface Targets (NXDevConfig.cmake)      |
|    Strict sysroot isolation: ${DEVKITPRO}/portlibs/switch  |
+-----------------------------------------------------------+
```

### Key Principles:
1. **Central Package Registry**: Versioned `package-registry/registry.json` validated by JSON Schema.
2. **Deterministic Resolution**: Graph resolution using 3-color DFS to guarantee absence of cycles and topologically order linker targets.
3. **devkitPro Pacman Backend**: Isolated adapter layer (`IPackageManagerBackend`, `DevkitProPacmanBackend`) supporting safe `--dry-run`, non-interactive execution, and hermetic mocking (`MockPackageManagerBackend`).
4. **Toolchain Sysroot Isolation**: All portlibs imported as interface targets linking strictly against Switch AArch64 sysroots, preventing host header or library pollution.

---

## 6. Run, Deploy & Runtime Diagnostics Subsystem Architecture

The **NXDev Run & Deploy** subsystem bridges the compilation pipeline with real Nintendo Switch hardware execution:

```
+-----------------------------------------------------------+
|                   NXDev CLI / NXDevToolkit                |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                 DeviceManager (.nxdev/devices.yaml)       |
|            Resolution: CLI args > Local > Global > Single |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                    RunSession / DeployService             |
|   1. Build ELF binary (nxdev build)                       |
|   2. Package NRO bundle (nxdev pack nro)                  |
|   3. Deploy / Upload via nxlink (port 28280)              |
|   4. Interactive Server / Log Stream (nxlink -s)          |
+-----------------------------------------------------------+
           /                                       \
          v                                         v
+-------------------------------+       +-------------------------------+
|     Live Stream / Logging     |       |    Symbolizer (addr2line)     |
|   stdout / stderr line stream |       |   aarch64-none-elf-addr2line  |
|   NDJSON events (--json-stream)       |   Demangles C++ function names|
|   Session log persistence     |       |   Resolves source file & line |
+-------------------------------+       +-------------------------------+
```

### Key Principles:
1. **Public Homebrew Tooling Only**: Leverages public `nxlink` (switch-tools) and `aarch64-none-elf-addr2line` (devkitA64). Zero proprietary debugging hooks or unofficial CFW modifications.
2. **Explicit Device Targeting**: Clear precedence chain for resolving target hardware without unprompted broadcast scanning.
3. **Structured Live Streaming**: Dual-mode terminal output: human-friendly colored progress and stdout streaming, or machine-readable NDJSON (`--json-stream`) for IDE integration.
4. **Crash Detection & Auto-Symbolization**: Real-time extraction of hex instruction pointers and automatic demangled stack frame resolution against the project's ELF binary.

---

## 7. Security & Intellectual Property Policy

NXDev strictly adheres to clean-room open-source principles:
- **Zero Proprietary Material**: Under no circumstances does NXDev include Nintendo SDK headers, title keys, prod keys, encrypted binaries, or copyright material.
- **Third-Party Licensing**: Third-party tools retain their upstream licenses and notices under `third_party/` and `THIRD_PARTY_NOTICES.md`.

