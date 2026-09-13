# NXDev Initial Release Scope (0.1.0-beta.1)

This document honestly outlines the readiness and maturity level of all subsystems in the initial release of NXDev.

---

## 1. Feature Classification

| Subsystem / Feature | Classification | Notes |
| :--- | :--- | :--- |
| **NXDevAppManifest Schema v1** | `Stable / Release-Ready` | Full parsing, strict validation, default fallback, and JSON Schema generation. |
| **NXDev CLI Core (`nxdev`)** | `Stable / Release-Ready` | Comprehensive subcommands: `new`, `doctor`, `env`, `config`, `manifest`, `package`, `configure`, `build`, `pack`, `devices`, `run`, `symbolize`. |
| **Project Creation (`nxdev new`)** | `Stable / Release-Ready` | Scaffolding from templates (`minimal-c`, `minimal-cpp`, `console-cpp`, `sdl2`, `deko3d`), variable substitution, directory safety. |
| **NXDevSDK Core** | `Stable / Release-Ready` | Error handling (`Result<T>`), RAII service context wrappers, logging system, console initialization. |
| **NXDevSDK Input** | `Stable / Release-Ready` | Pad state, button pressed/held/released edge triggers, analog stick deadzones and normalization. |
| **NXDevSDK Filesystem** | `Stable / Release-Ready` | RomFS mounting/lifecycle, path utilities, safe atomic writes, directory scanning. |
| **NXDevSDK Account** | `Stable / Release-Ready` | User profile enumeration, active user query, profile nicknames. |
| **NXDevSDK Network** | `Stable / Release-Ready` | Socket service initialization, multi-context lifetime management, basic networking wrappers. |
| **NXDevSDK Time** | `Stable / Release-Ready` | Monotonic clock, POSIX time conversion, tick-to-duration conversions. |
| **NXDevSDK Power** | `Stable / Release-Ready` | Operation mode (docked/handheld), battery charge level, system power events. |
| **Package / Dependency System** | `Stable / Release-Ready` | Registry mapping to devkitPro pacman packages, status checking, conservative missing dependency installation. |
| **NXDevPack NRO Packaging** | `Stable / Release-Ready` | NACP control metadata generation, custom/default icon resolution, RomFS bundling, `elf2nro` invocation. |
| **NXDev Deploy & Run (`nxdev run`)**| `Stable / Release-Ready` | Wireless `nxlink` deployment, device configuration, log capture, `--json-stream` events. |
| **NXDevToolkit (VS Code)** | `Beta` | Manifest YAML completion/validation, project tree view, device picker, task integration, terminal orchestration. |
| **Crash Symbolization (`nxdev symbolize`)** | `Experimental` | Stack trace offset parsing via `aarch64-none-elf-addr2line`. Accuracy depends on unstripped debug ELF availability. |
| **NXDevPack NSP Packaging** | `Experimental` | NPDM generation, ExeFS preparation, pinned `hacBrewPack` backend. Requires user-supplied `prod.keys`. |

---

## 2. Known Limitations

1. **NSP Cryptographic Keys**:
   - NXDev does **NOT** redistribute Nintendo cryptographic keys (`prod.keys`, `title.keys`). Real NSP generation requires users to provide their own dumped keys. Staging and fake backend tests function without keys.
2. **Real Hardware vs. Local Mocks**:
   - Integration tests in CI run against mocked toolchain binaries and local loopback sockets. Real-device wireless `nxlink` deployment requires an actual Nintendo Switch on the local subnet.
3. **Host Platforms**:
   - Primary supported host: **Linux (x86_64 / AArch64)** and **Windows via WSL2**.
   - Native Windows: Builds CMake and host tools, but devkitPro cross-compilation is best supported under WSL2.
   - macOS: Experimental due to devkitPro packaging differences on macOS ARM64.

---

## 3. Future Roadmap (Post-0.1.0)

- Integrated GDB-over-nxlink remote debugging protocol.
- Automated emulator launch integration (Ryujinx/yuzu test harnesses).
- Multi-target workspace support for multi-module Switch applications.
- Additional package registry feeds and custom vendor repositories.
