# NXDevToolkit for Visual Studio Code

**NXDevToolkit** is the official Visual Studio Code integration layer for the **NXDev** Nintendo Switch homebrew development ecosystem.

It provides a polished, interactive IDE experience over the underlying `nxdev` CLI, giving developers seamless access to project orchestration, manifest diagnostics, SDK package management, compilation pipelines, and NRO/NSP packaging.

---

## Architectural Principles

1. **CLI as the Single Source of Truth**: NXDevToolkit does not reimplement or duplicate CMake generation, toolchain discovery, manifest validation rules, or packaging backends in JavaScript. Instead, it interacts with the `nxdev` CLI via standardized JSON protocols.
2. **Transparent Command Execution**: All build, package, and dependency actions stream directly to the dedicated **NXDev** Output Channel in VS Code for complete auditability.
3. **Workspace Trust Enforced**: Build and packaging operations are guarded by VS Code Workspace Trust to protect against executing untrusted code.
4. **Cross-Platform & WSL Native**: Full support for native Linux and Windows with WSL (Windows Subsystem for Linux), including automated path translation (`C:\` ↔ `/mnt/c/`).
5. **Zero Bundled Keys**: In strict compliance with NXDev security and legal boundaries, NXDevToolkit never bundles, stores, or transmits proprietary Nintendo encryption keys.

---

## Features

- **Activity Bar & Explorer**:
  - **NXDev Project**: View active project identity, target, title ID, version, author, and RomFS/icon configuration.
  - **Environment & Diagnostics**: Real-time devkitPro / devkitA64 toolchain discovery status, switch-tools, and `nxdev doctor` integration.
  - **SDK Modules & Dependencies**: Visual package browser categorized by domain (Core, Graphics, Audio, Network, Media, Storage). One-click install/remove.
- **Rich Manifest Editing (`nxapp.yaml`)**:
  - Schema auto-completion and tooltips via JSON Schema.
  - Real-time semantic diagnostics from `nxdev manifest validate --json`.
  - QuickFix Code Actions for missing required fields or uninstalled dependencies.
- **One-Click Build & Packaging**:
  - Build Debug / Release Switch ELFs.
  - Package `.nro` homebrew executables.
  - Package `.nsp` installable packages using the built-in pinned hacBrewPack backend.
- **Native VS Code Tasks**:
  - Seamless integration with `Ctrl+Shift+B` (Build) and custom tasks configured in `tasks.json`.
- **Status Bar Integration**:
  - Live status indicator displaying current build profile (`[Debug]` / `[Release]`) and busy states (`Building...`, `Packaging...`).

---

## Documentation Index

- [Getting Started](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/getting-started.md)
- [Manifest Editing & Diagnostics](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/manifest.md)
- [Building Applications](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/building.md)
- [Packaging NRO and NSP](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/packaging.md)
- [SDK Modules & Dependencies](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/sdk-modules.md)
- [Windows & WSL Setup](file:///home/jvrcruz/Projects/NXDev/docs/toolkit/wsl.md)
