# Compatibility & Environment Matrix

This document defines the verified toolchain versions, host operating systems, and target dependencies for NXDev.

---

## 1. Host Operating Systems

| Host OS | Status | Recommended Workflow |
| :--- | :--- | :--- |
| **Ubuntu 22.04 / 24.04 LTS (x86_64)** | **Tier 1 (Fully Supported)** | Native devkitPro and native GCC/Clang. |
| **Debian 12 / Fedora 39 / Arch Linux** | **Tier 1 (Fully Supported)** | Native devkitPro. |
| **Windows 10 / 11 via WSL2** | **Tier 1 (Recommended for Windows)** | devkitPro inside Ubuntu WSL2 + VS Code Remote WSL. |
| **Windows 10 / 11 (Native)** | **Tier 2 (Supported with Limitations)** | Native CMake/MSVC builds CLI; cross-compiling Switch ELF requires devkitPro MSYS2 environment. |
| **macOS 13+ (Apple Silicon / Intel)** | **Tier 2 (Community Supported)** | Homebrew devkitPro pacman packages. |

---

## 2. Toolchain & Compilers

| Tool | Minimum Version | Verified Version | Notes |
| :--- | :--- | :--- | :--- |
| **devkitA64** | `r17-1` | `r20` | aarch64-none-elf GCC 13+ / 14+. |
| **libnx** | `v4.5.0` | `v4.6.0` | Target Nintendo Switch OS abstraction. |
| **switch-tools** | `v1.11.0` | `v1.12.0` | Includes `elf2nro`, `nacptool`, `nxlink`. |
| **CMake** | `3.20.0` | `3.28.3` | Build configuration and target generators. |
| **Host C++ Compiler** | C++20 compliant | GCC 11+, Clang 13+, MSVC 19.30+ | Required for building NXDev CLI and host tools. |
| **Target C++ Compiler** | C++20 compliant | `aarch64-none-elf-g++` 13+ | Required for building NXDevSDK Switch applications. |
| **Node.js** | `18.0.0` | `20.x LTS` | Required for NXDevToolkit VS Code extension. |
| **VS Code** | `1.80.0` | `1.90.0+` | Required for NXDevToolkit extension. |

---

## 3. Pinned Components & Libraries

| Component | Upstream / Revision | License | Notes |
| :--- | :--- | :--- | :--- |
| **hacBrewPack** | Revision `2.0.0` (pinned) | ISC | NSP packaging backend (packaged as source/submodule). |
| **yaml-cpp** | `0.8.0` | MIT | Manifest parser. |
| **nlohmann/json** | `3.11.3` | MIT | JSON schema, doctor, and stream serialization. |
