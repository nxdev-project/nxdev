# NXDev Project Templates

This directory contains official starter templates for creating new Nintendo Switch homebrew applications with `nxdev new`.

## Available Templates

| Template ID | Name | Language | Dependencies | Description |
|---|---|---|---|---|
| **`minimal-c`** | Minimal C | C11 | Raw libnx | Clean C starting point demonstrating raw libnx console and input |
| **`minimal-cpp`** | Minimal C++ | C++20 | Raw libnx | C++20 starting point using raw libnx without NXDevSDK wrappers |
| **`console-cpp`** | NXDevSDK C++ | C++20 | `nxdev.core`, `nxdev.input` | Recommended starting point using modern NXDevSDK abstractions |
| **`sdl2`** | SDL2 App | C++20 | `nxdev.core`, `nxdev.sdl2` | 2D graphics and windowed application using devkitPro SDL2 portlib |
| **`deko3d`** | deko3d App | C++20 | `nxdev.core`, `nxdev.deko3d` | Low-level 3D graphics using Switch native deko3d API (Experimental) |

## Template Manifest

Template metadata is authoritatively declared in [`manifest.json`](manifest.json) and consumed by both `nxdev new` and the VS Code `NXDevToolkit` extension.
