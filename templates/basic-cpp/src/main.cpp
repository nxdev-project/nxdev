#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <iostream>

int main() {
    nxdev::App app;
    if (auto res = app.initialize(); res.is_failure()) {
        std::cerr << "Failed to initialize NXDev app: " << res.get_error_name() << "\n";
        return 1;
    }

    nxdev::input::InputManager input;
    input.initialize();

    while (app.is_running()) {
        input.poll();
        if (input.is_pressed(nxdev::input::Button::Plus)) {
            app.request_exit();
        }
    }

    input.finalize();
    app.finalize();
    return 0;
}
