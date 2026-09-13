# Application Lifecycle (`nxdev::App`)

The `nxdev::App` class provides a clean RAII manager for Nintendo Switch homebrew application lifecycles.

## Overview

Under Horizon OS, homebrew applications run inside an applet or application container. The runtime lifecycle relies on:
- `appletMainLoop()`: Polling the Horizon OS windowing and process event loop.
- `appletGetAppletType()`: Determining whether the process runs as an Application (title takeover), SystemApplet, or LibraryApplet (applet mode).

`nxdev::App` wraps this lifecycle seamlessly, providing:
1. Safe RAII initialization and finalization.
2. Clean integration with `appletMainLoop()`.
3. Non-blocking exit requests via `request_exit()`.
4. Host/mock execution support for continuous integration and unit testing.

---

## Usage

### Factory Initialization

```cpp
#include <nxdev/app.hpp>
#include <nxdev/log.hpp>

int main(int argc, char* argv[]) {
    // Create and initialize the application lifecycle
    auto app_res = nxdev::App::create();
    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize NXDev App!");
        return static_cast<int>(app_res.code().raw());
    }
    auto& app = app_res.value();

    nxdev::log::info("App initialized. Type: " + std::string(nxdev::applet_type_to_string(app.applet_type())));

    // Application loop:
    while (app.running()) {
        // Handle input, update simulation, render frame...

        if (user_pressed_exit) {
            app.request_exit();
        }
    }

    return 0;
}
```

### Methods Reference

| Method | Return Type | Description |
|--------|-------------|-------------|
| `App::create()` | `Result<App>` | Factory creating and initializing a new `App` instance. |
| `initialize()` | `Result<void>` | Initializes SDK subsystems. Returns `AlreadyInitialized` if called more than once. |
| `running()` | `bool` | Returns `true` while the application should continue running. Directly delegates to `appletMainLoop()` on Switch. |
| `request_exit()` | `void` | Signals the main loop to terminate gracefully. |
| `exit_requested()` | `bool` | Returns `true` if an exit has been requested. |
| `applet_type()` | `AppletType` | Returns the active Horizon OS applet type (`Application`, `LibraryApplet`, `SystemApplet`, etc.). |
| `finalize()` | `void` | Shuts down subsystems and triggers clean exit. Called automatically on destruction. |
