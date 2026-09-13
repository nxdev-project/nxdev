# libnx Interoperability & RAII

A fundamental design requirement of NXDev is seamless, frictionless coexistence with raw `libnx` C APIs (`switch.h`). Developers are never forced to choose between the high-level C++ SDK and direct hardware/OS control.

## Including `<switch.h>`

You can safely include `<switch.h>` alongside `<nxdev/nxdev.hpp>`:

```cpp
#include <nxdev/nxdev.hpp>
#include <switch.h>

int main(int argc, char* argv[]) {
    auto app = nxdev::App::create();
    if (app.failed()) return 1;

    // Use raw libnx APIs directly:
    consoleInit(nullptr);
    
    // RAII clean-up:
    auto console_guard = nxdev::scope_exit([]() {
        consoleExit(nullptr);
    });

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    while (app->running()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus) {
            break;
        }

        consoleUpdate(nullptr);
    }

    return 0;
}
```

---

## RAII Helpers

### `nxdev::scope_exit`

Guarantees execution of a cleanup closure when exiting scope, with optional cancellation.

```cpp
#include <nxdev/scope_exit.hpp>

void process_file() {
    FILE* fp = fopen("sdmc:/test.bin", "rb");
    if (!fp) return;

    auto file_guard = nxdev::scope_exit([fp]() {
        fclose(fp);
    });

    // If an early return occurs, fclose is called automatically.
    if (!validate_header(fp)) {
        return;
    }

    // You can also cancel the exit guard if ownership is transferred:
    // file_guard.cancel();
}
```

---

### `nxdev::ServiceGuard` and `make_service_guard`

Manages the lifecycle of Horizon OS services (`fsInitialize`/`fsExit`, `socketInitializeDefault`/`socketExit`, etc.).

```cpp
#include <nxdev/service.hpp>
#include <switch.h>

nxdev::Result<void> init_networking() {
    // Initializes socket service and registers socketExit() for RAII cleanup
    auto socket_guard = NXDEV_TRY(nxdev::make_service_guard(
        socketInitializeDefault,
        socketExit
    ));

    // Networking operations...

    // socketExit() is automatically invoked when socket_guard goes out of scope.
    return nxdev::Result<void>::Success();
}
```
