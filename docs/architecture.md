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
|   (Modern C++ abstractions: App, Input, Display, Audio)   |
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
   - Provides clean object-oriented abstractions (`nxdev::App`, `nxdev::input`, `nxdev::display`, `nxdev::Result`).
   - Does **not** depend on the editor or the CLI.
   - Designed to compile as stubs/mocks on host environments and bind to native libnx when targeting Nintendo Switch.

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

## 5. Security & Intellectual Property Policy

NXDev strictly adheres to clean-room open-source principles:
- **Zero Proprietary Material**: Under no circumstances does NXDev include Nintendo SDK headers, title keys, prod keys, encrypted binaries, or copyright material.
- **Third-Party Licensing**: Third-party tools retain their upstream licenses and notices under `third_party/` and `THIRD_PARTY_NOTICES.md`.
