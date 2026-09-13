# Power & Application State (`NXDev::Power`)

The `NXDev::Power` module provides safe query and event notification facilities for Horizon OS operation modes (docked/handheld), focus state, battery status, and applet hooks.

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.power
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Power)
```

## Header

```cpp
#include <nxdev/power.hpp>
```

---

## State Queries

```cpp
#include <nxdev/power.hpp>
#include <iostream>

void check_system_state() {
    // Operation mode:
    nxdev::power::OperationMode mode = nxdev::power::get_operation_mode();
    if (mode == nxdev::power::OperationMode::Docked) {
        std::cout << "Console is currently Docked.\n";
    } else {
        std::cout << "Console is in Handheld mode.\n";
    }

    // Focus state:
    nxdev::power::FocusState focus = nxdev::power::get_focus_state();
    // InFocus, OutOfFocus, Background

    // Battery charge percentage:
    auto bat_res = nxdev::power::get_battery_info();
    if (bat_res.is_success()) {
        std::cout << "Battery: " << bat_res->percentage << "% (Charging: " 
                  << (bat_res->is_charging ? "Yes" : "No") << ")\n";
    }
}
```

---

## RAII Event Hooks

Register callbacks for Horizon OS lifecycle notifications without manual unhooking:

```cpp
void setup_lifecycle_hooks() {
    auto hook = nxdev::power::register_event_handler([](nxdev::power::AppletEvent ev) {
        switch (ev) {
            case nxdev::power::AppletEvent::OperationModeChanged:
                std::cout << "Docking mode changed!\n";
                break;
            case nxdev::power::AppletEvent::FocusChanged:
                std::cout << "Application focus state changed.\n";
                break;
            case nxdev::power::AppletEvent::Resumed:
                std::cout << "Application resumed from sleep.\n";
                break;
            default:
                break;
        }
    });

    // The hook is automatically unregistered when `hook` is destroyed.
}
```
