# NXDevSDK

**NXDevSDK** is a modern C++20 abstraction SDK designed specifically for Nintendo Switch homebrew development.

## Philosophy

1. **Complement, Never Replace libnx**:
   libnx is the bedrock of Nintendo Switch homebrew. NXDevSDK builds on top of libnx, providing idiomatic modern C++ types, RAII resource lifecycle management, structured error handling, and robust logging, without hiding or interfering with raw libnx APIs.

2. **Zero-Cost Abstractions & Zero Overhead**:
   Where possible, types compile down to direct Horizon OS syscalls or direct libnx structures without unnecessary heap allocations or runtime penalties.

3. **No Exceptions**:
   Because embedded game consoles and homebrew environments often compile with `-fno-exceptions` or avoid exception runtime overhead, all operations communicate success/failure through typed, `[[nodiscard]]` `nxdev::Result<T>` and `nxdev::Result<void>`.

4. **Horizon OS Result Code Preservation**:
   All 32-bit Horizon OS module/description error codes are preserved intact, enabling full error diagnostics and interoperability.

## SDK Structure

| Component | Target Name | Manifest Dep | Description |
|-----------|-------------|--------------|-------------|
| **Core** | `NXDev::Core` | `nxdev.core` | Fundamental abstractions: `Result<T>`, `App` lifecycle, `ScopeExit`, `ServiceGuard`, `log`, `system`, and `version`. |
| **Input** | `NXDev::Input` | `nxdev.input` | Gamepad, Joy-Con, Pro Controller, analog sticks, dead zones, and touch input on top of libnx `PadState` and `hid`. |
| **Filesystem** | `NXDev::Filesystem` | `nxdev.filesystem` | RomFS RAII mount lifecycle, safe file reading/writing with size safety, and canonical path helpers. |
| **Account** | `NXDev::Account` | `nxdev.account` | Horizon local user enumeration, last opened/preselected user, and user profile metadata (`acc:*`). |
| **Network** | `NXDev::Network` | `nxdev.network` | BSD socket driver RAII lifecycle (`bsd:*`) with reference counting and local IP query. |
| **Time** | `NXDev::Time` | `nxdev.time` | Monotonic clock (`std::chrono`), hardware AArch64 system ticks (19.2 MHz), calendar date/time, and sleep helpers. |
| **Power** | `NXDev::Power` | `nxdev.power` | Operation mode (docked/handheld), focus state, battery status, and RAII applet event hooks. |

## Documentation Index

- **[Core Overview](core.md)**
- **[Input & Controller Management](input.md)**
- **[Filesystem & RomFS](filesystem.md)**
- **[User & Account Management](account.md)**
- **[Network & Sockets](network.md)**
- **[Time & System Ticks](time.md)**
- **[Power & Application State](power.md)**
- **[Result & Error Handling](result.md)**
- **[Application Lifecycle](app.md)**
- **[Logging Subsystem](logging.md)**
- **[libnx Interoperability & RAII](interoperability.md)**

