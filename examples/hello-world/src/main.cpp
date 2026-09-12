#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <nxdev/display.hpp>
#include <iostream>

int main() {
    std::cout << "Starting NXDev Hello World application...\n";

    nxdev::App app;
    if (auto res = app.initialize(); res.is_failure()) {
        std::cerr << "Initialization failed: " << res.get_error_name() << "\n";
        return 1;
    }

    nxdev::input::InputManager input;
    input.initialize();

    nxdev::display::DisplayManager display;
    display.initialize();

    auto res_info = display.get_resolution();
    std::cout << "Display resolution: " << res_info.width << "x" << res_info.height << "\n";
    std::cout << "App running. Press PLUS to exit (or automatic termination in host mode).\n";

    // Simulate 1 loop step in host mode
    if (app.is_running()) {
        input.poll();
        std::cout << "NXDev App lifecycle active.\n";
        app.request_exit();
    }

    input.finalize();
    display.finalize();
    app.finalize();

    std::cout << "Application exited gracefully.\n";
    return 0;
}
