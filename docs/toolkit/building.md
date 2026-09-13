# Building Applications in NXDevToolkit

NXDev provides a streamlined, CMake-driven compilation pipeline targeting the Nintendo Switch (AArch64 `aarch64-none-elf` via devkitA64 and libnx).

---

## Build Profiles

NXDev supports two primary build profiles:
- **Debug**: Optimization `-O0` or `-Og`, complete debug symbols enabled (`-g3`), runtime assertions enabled.
- **Release**: Optimization `-O3`, stripped debug symbols, release assertions.

To switch profiles:
- Click the profile indicator in the Status Bar (`NXDev [Debug]`), or
- Run `NXDev: Select Build Profile (Debug/Release)` from the Command Palette.

---

## Building

- **Command**: `NXDev: Build Application (Active Profile)`
- **Keybinding**: `Ctrl+Shift+B` / `Cmd+Shift+B`
- **Output**: The compilation log streams in real time to the **NXDev** Output Channel. The resulting Switch ELF is placed into `build/<profile>/<target>.elf`.

---

## Configuring & Cleaning

- **Configure CMake**: Run `NXDev: Configure CMake` to force CMake generation/cache refresh.
- **Clean**: Run `NXDev: Clean Build Artifacts` to purge the build directory for the active profile.

---

## VS Code Task Integration

NXDevToolkit registers custom task providers of type `nxdev`. You can define custom tasks in `.vscode/tasks.json`:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "type": "nxdev",
      "task": "build",
      "profile": "release",
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": ["$gcc"]
    }
  ]
}
```
