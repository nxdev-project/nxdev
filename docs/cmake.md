# NXDev CMake Integration Guide

NXDev provides modern CMake integration for Nintendo Switch homebrew applications, encapsulating devkitA64 cross-compilers, libnx system headers, Tegra X1 architecture flags, and linker specifications.

---

## Toolchain File: `NXDevSwitch.cmake`

Path: `cmake/toolchains/NXDevSwitch.cmake`

The toolchain file configures CMake for cross-compiling to the Nintendo Switch:
- `CMAKE_SYSTEM_NAME`: `Generic`
- `CMAKE_SYSTEM_PROCESSOR`: `aarch64`
- `CMAKE_TRY_COMPILE_TARGET_TYPE`: `STATIC_LIBRARY`
- **Compiler Flags**: `-march=armv8-a+crc+crypto -mtune=cortex-a57 -mcpu=cortex-a57 -fPIE -ftls-model=local-exec`

---

## Package Configuration: `find_package(NXDev REQUIRED)`

When included in a project's `CMakeLists.txt`, `NXDevConfig.cmake` provides:

### 1. Target: `NXDev::Libnx`

An imported interface target that provides:
- **Include Directories**: `${DEVKITPRO}/libnx/include`
- **Definitions**: `-D__SWITCH__`
- **Architecture Flags**: Tegra X1 Cortex-A57 ARMv8-A options
- **Linker Flags**: `-fPIE -pie -specs=${DEVKITPRO}/libnx/switch.specs`
- **Libraries**: `-lnx -lm`

### 2. Helper Function: `nxdev_add_application`

Creates a Nintendo Switch homebrew executable target and applies all required linkage:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_homebrew_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(NXDev REQUIRED)

nxdev_add_application(my_homebrew_app
    SOURCES
        src/main.cpp
        src/game.cpp
)
```

---

## Standalone CMake Invocation

While `nxdev build` is the recommended frontend, projects can also be built directly with CMake:

```bash
cmake -S . -B build/switch \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/nxdev/cmake/toolchains/NXDevSwitch.cmake \
  -DNXDev_DIR=/path/to/nxdev/cmake \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/switch
```
