# Contributing to NXDev

Thank you for your interest in contributing to NXDev! This guide explains how to set up your local development environment, build the monorepo, and run tests.

---

## 1. Prerequisites

### Host Build Tools:
* **CMake** >= 3.20
* **C++20 Compiler** (GCC 11+, Clang 13+, or MSVC 2022)
* **Ninja** or GNU Make
* **Node.js** >= 18.0 & **npm** (for `NXDevToolkit` VS Code extension)

### Optional Switch Cross-Compilation:
* **devkitPro** with `devkitA64`, `libnx`, and `switch-tools` (e.g. at `/opt/devkitpro`).

---

## 2. Developer Bootstrap

```bash
# Clone the repository
git clone https://github.com/nxdev-project/nxdev.git
cd nxdev

# 1. Configure and build native host CLI and test suite
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# 2. Run the C++ test suite
ctest --test-dir build --output-on-failure

# 3. Build and test the VS Code extension
cd toolkit/vscode
npm install
npm test
npm run lint
```

---

## 3. Architecture Guidelines

NXDev enforces strict dependency layering:

1. **`NXDevToolkit` (VS Code Extension)**: Thin TypeScript UI orchestrator. Depends exclusively on the CLI via process execution and JSON IPC.
2. **`NXDev CLI`**: Native C++ command-line interface. Orchestrates builds, packaging, packages, and hardware deploy sessions.
3. **`NXDevPack`**: Multi-backend packaging engine (`NroPackBackend`, `NspPackBackend`). Isolates third-party tool adapters.
4. **`NXDevAppManifest`**: Declarative manifest specification (`nxapp.yaml`) and semantic validator.
5. **`NXDevSDK`**: Standalone C++20 platform abstractions. Pure native code with zero dependencies on the CLI or IDE.

---

## 4. Coding & Formatting Standards

* **C++**: Follow C++20 best practices, RAII resource ownership, and `clang-format`.
* **TypeScript**: Use ESLint and Prettier (`npm run lint` and `npm run format` in `toolkit/vscode`).
* **Pull Requests**: All CI checks (host builds, ctest, npm test, and linter) must pass before merging.
