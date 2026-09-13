# Time & System Ticks (`NXDev::Time`)

The `NXDev::Time` module provides high-resolution monotonic clocks, hardware system tick conversions, calendar date/time utilities, and duration sleep helpers.

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.time
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Time)
```

## Header

```cpp
#include <nxdev/time.hpp>
```

---

## High-Resolution Monotonic Clock

`nxdev::time::Clock` is a `std::chrono`-compatible steady monotonic clock for frame timing and benchmarks:

```cpp
#include <nxdev/time.hpp>

void frame_loop() {
    auto start = nxdev::time::Clock::now();

    // Render frame...

    auto end = nxdev::time::Clock::now();
    std::chrono::duration<float, std::milli> frame_time = end - start;
    std::cout << "Frame time: " << frame_time.count() << " ms\n";
}
```

---

## Hardware System Ticks

Nintendo Switch ARM Cortex-A57 timer ticks run at 19.2 MHz:

```cpp
// Read raw hardware timer tick (via armGetSystemTick / cntpct_el0):
u64 tick = nxdev::time::get_system_tick();

// Convert ticks <-> nanoseconds:
u64 ns = nxdev::time::ticks_to_nanoseconds(tick);
u64 back_to_ticks = nxdev::time::nanoseconds_to_ticks(ns);
```

---

## Calendar Wall-Clock & ISO 8601 Formatting

```cpp
nxdev::time::DateTime dt = nxdev::time::wall_clock_now();
// dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.millisecond

std::string iso = dt.format_iso8601(); // e.g. "2026-09-12T22:15:30Z"
```

---

## Sleep Helper

```cpp
using namespace std::chrono_literals;
nxdev::time::sleep_for(16ms);
```
