# Declaring and Using Project Dependencies

This guide explains how to declare dependencies in an NXDev application manifest (`nxapp.yaml`) and link against them in CMake.

---

## Declaring Dependencies in `nxapp.yaml`

Applications declare both built-in NXDev modules and external portlib dependencies in the `dependencies` list in `nxapp.yaml`:

```yaml
schemaVersion: 1

application:
  titleId: "0100000000000042"
  name: "MySwitchApp"
  author: "Developer"
  version: "1.0.0"
  description: "An SDL2 and Deko3D application on Nintendo Switch"

dependencies:
  - nxdev.core
  - nxdev.input
  - nxdev.sdl2
  - nxdev.sdl2-image
  - nxdev.deko3d

build:
  language: cpp
  standard: "c++20"
  defaultProfile: debug

packaging:
  defaultFormat: nro
  nro:
    enabled: true
```

### Module Aliases and Short Names

The dependency resolver automatically handles aliases:
- Full ID: `nxdev.sdl2`
- Short Name: `sdl2`
- devkitPro package name: `switch-sdl2`

All resolve correctly to `nxdev.sdl2`.

---

## Linking in CMake

When `NXDevConfig.cmake` is loaded (via `find_package(NXDev REQUIRED)`), imported CMake interface targets are automatically created for all installed modules:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MySwitchApp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(NXDev REQUIRED)

add_executable(my_app src/main.cpp)

target_link_libraries(my_app PRIVATE
    NXDev::Core
    NXDev::Input
    NXDev::SDL2
    NXDev::SDL2Image
    NXDev::Deko3D
)

nxdev_configure_target(my_app)
```

---

## Cross-Compilation Isolation

NXDev guarantees strict toolchain isolation when cross-compiling for Nintendo Switch:
- The base `NXDev::Portlibs` interface target adds `-isystem ${DEVKITPRO}/portlibs/switch/include` and `-L${DEVKITPRO}/portlibs/switch/lib`.
- Module targets (e.g. `NXDev::SDL2`, `NXDev::Curl`) link directly against Switch static archives (e.g., `libSDL2.a`, `libcurl.a`).
- Host `/usr/include` and `/usr/lib` paths are never referenced or linked.
