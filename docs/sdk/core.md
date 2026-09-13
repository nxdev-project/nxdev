# NXDevSDK Core (`NXDev::Core`)

The `NXDev::Core` library provides the foundational C++ types and lifecycle abstractions for Nintendo Switch homebrew applications.

## Linking `NXDev::Core`

To link against `NXDev::Core` in your `CMakeLists.txt`:

```cmake
find_package(NXDev REQUIRED)

# When using standard CMake executable:
target_link_libraries(myapp PRIVATE NXDev::Core)

# When using nxdev_add_application helper, NXDev::Core is linked automatically:
nxdev_add_application(myapp
    SOURCES
        src/main.cpp
)
```

## Available Headers

Include all core headers at once:
```cpp
#include <nxdev/nxdev.hpp>
```

Or include specific headers as needed:
```cpp
#include <nxdev/types.hpp>      // Fixed-width integer aliases (u8, u16, u32, u64, etc.)
#include <nxdev/result.hpp>     // Result<T>, Result<void>, ResultCode, NXDEV_TRY
#include <nxdev/scope_exit.hpp> // ScopeExit, scope_exit()
#include <nxdev/service.hpp>    // ServiceGuard, make_service_guard()
#include <nxdev/app.hpp>        // App, AppletType
#include <nxdev/log.hpp>        // log::info, log::warn, log::error, ILogSink
#include <nxdev/system.hpp>     // system::get_firmware_version, memory queries
#include <nxdev/version.hpp>    // version(), SdkVersion
```

## System & Version APIs

### Firmware and System Queries

```cpp
#include <nxdev/system.hpp>

// Firmware version:
nxdev::system::FirmwareVersion fw = nxdev::system::get_firmware_version();
// fw.major, fw.minor, fw.micro, fw.display_version (e.g. "17.0.1")

// Applet execution type under Horizon OS:
nxdev::AppletType applet = nxdev::system::get_applet_type();

// System memory tracking (in bytes):
u64 total_bytes = nxdev::system::get_total_memory();
u64 used_bytes  = nxdev::system::get_used_memory();
```

### SDK Version

```cpp
#include <nxdev/version.hpp>

nxdev::SdkVersion ver = nxdev::version();
// ver.major, ver.minor, ver.patch, ver.prerelease, ver.string ("0.1.0-dev")
```
