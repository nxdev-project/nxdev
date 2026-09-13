# NXDev Package & Pluggable SDK Subsystem

The **NXDev Package Subsystem** provides a modular, declarative dependency management layer for Nintendo Switch homebrew development. It bridges high-level module declarations in application manifests (`nxapp.yaml`) with the underlying Nintendo Switch devkitPro portlib ecosystem and NXDev SDK C++ abstractions.

---

## Key Features

1. **Declarative Dependencies**:
   Applications declare capabilities (e.g., `nxdev.core`, `nxdev.input`, `nxdev.sdl2`, `nxdev.sdl2-image`, `nxdev.deko3d`, `nxdev.curl`) in `nxapp.yaml`.
2. **Unified Registry**:
   A single, extensible JSON-based package registry (`package-registry/registry.json`) with JSON Schema validation (`package-registry/schema/package-registry.schema.json`).
3. **Built-in vs. Portlib Separation**:
   - **Built-in modules**: Core NXDev C++ abstractions shipped in-tree (`nxdev.core`, `nxdev.input`, `nxdev.filesystem`, `nxdev.account`, `nxdev.network`, `nxdev.time`, `nxdev.power`, `nxdev.display`).
   - **devkitPro portlibs**: Official portlib packages (`switch-sdl2`, `switch-sdl2_image`, `switch-curl`, `deko3d`, etc.) installed into `$DEVKITPRO/portlibs/switch`.
4. **Dependency Graph Resolution**:
   Transitive dependency resolution, cycle detection (DFS 3-color), deduplication, and topological sorting.
5. **dkp-pacman Integration**:
   Non-interactive and dry-run query, install, and removal via devkitPro's `pacman` wrapper.
6. **Cross-Compilation Isolation**:
   CMake interface targets (`NXDev::Portlibs`, `NXDev::SDL2`, `NXDev::Deko3D`, etc.) link exclusively against `$DEVKITPRO/portlibs/switch` and `$DEVKITPRO/libnx`, never host system libraries.
7. **Pre-Build Verification & Fingerprinting**:
   Automatic checking before CMake configure/build steps with `.nxdev/dependencies.fingerprint` caching.

---

## Documentation Index

- [Package Registry Architecture & Schema](registry.md) — Registry schema, field specification, and discovery rules.
- [Declaring Dependencies in Applications](dependencies.md) — How to declare dependencies in `nxapp.yaml` and CMake usage.
- [devkitPro Package Management](devkitpro.md) — CLI commands (`nxdev package install`, `list`, `status`), dry-run, and troubleshooting.
- [Supported Module Reference](modules.md) — Comprehensive reference of all 23 built-in and devkitPro portlib modules.

---

## Quick CLI Reference

```bash
# List all available modules and installation status
nxdev package list

# Inspect module metadata and dependencies
nxdev package info nxdev.sdl2

# Check dependencies declared by the active project
nxdev package status

# Search available modules
nxdev package search sdl

# Dry-run missing dependency installation
nxdev package install --missing --dry-run

# Install missing project dependencies
nxdev package install --missing
```
