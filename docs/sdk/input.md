# Input & Controller Management (`NXDev::Input`)

The `NXDev::Input` module provides modern C++ abstractions for Nintendo Switch gamepad, Joy-Con, Pro Controller, and touch screen input, built directly on top of libnx's modern `PadState` and `hid` APIs.

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.input
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Input)
```

## Header

```cpp
#include <nxdev/input.hpp>
```

---

## Polling & Controller Abstraction

Input state is polled explicitly per-frame using `Controller::update()`:

```cpp
#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>

int main() {
    auto app = nxdev::App::create();
    if (app.failed()) return 1;

    // Supports Player::One..Eight, Player::Handheld, or Player::Auto
    nxdev::input::Controller controller(nxdev::input::Player::Auto);

    while (app->running()) {
        controller.update();

        // Button checks (zero dynamic allocations):
        if (controller.pressed(nxdev::input::Button::Plus)) {
            app->request_exit();
            break;
        }

        if (controller.down(nxdev::input::Button::A)) {
            // Button A is held down
        }

        if (controller.released(nxdev::input::Button::B)) {
            // Button B was just released on this frame
        }

        // Analog stick queries:
        nxdev::input::Stick left_stick = controller.left_stick();
        // Normalized values in range [-1.0f, 1.0f]:
        float lx = left_stick.x;
        float ly = left_stick.y;

        // Apply custom deadzone (e.g. 0.15f):
        auto filtered = left_stick.apply_deadzone(0.15f);

        // Controller style & connection status:
        if (controller.is_connected()) {
            nxdev::input::ControllerStyle style = controller.style();
            // ProController, Handheld, JoyConPair, JoyConLeft, JoyConRight, etc.
        }
    }

    return 0;
}
```

---

## Touch Screen Input

To query active multi-touch contact points:

```cpp
auto touch_points = nxdev::input::touches();
for (const auto& touch : touch_points) {
    // touch.id, touch.x, touch.y, touch.diameter_x, touch.diameter_y
}
```

---

## Raw libnx Interoperability

Developers can obtain a direct pointer to the underlying `PadState` struct at any time:

```cpp
#ifdef __SWITCH__
PadState* raw_pad = static_cast<PadState*>(controller.native_pad());
#endif
```
