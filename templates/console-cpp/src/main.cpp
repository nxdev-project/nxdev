#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <iostream>

#ifdef __SWITCH__
#include <switch.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Starting {{PROJECT_NAME}}...");

    auto app_res = nxdev::App::create();
    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize NXDev App!");
        return static_cast<int>(app_res.code().raw());
    }
    auto& app = app_res.value();

#ifdef __SWITCH__
    consoleInit(nullptr);
    auto console_guard = nxdev::scope_exit([]() {
        consoleExit(nullptr);
    });

    std::cout << "\x1b[2;2H=== " << "{{PROJECT_NAME}}" << " ===\n";
    std::cout << "\x1b[4;2HPowered by NXDevSDK (NXDev::Core + NXDev::Input)\n";
    std::cout << "\x1b[6;2HPress PLUS (+) to exit.\n";
#endif

    nxdev::input::Controller controller(nxdev::input::Player::Auto);

    while (app.is_running()) {
        controller.update();

        if (controller.is_button_down(nxdev::input::Button::Plus)) {
            app.request_exit();
            break;
        }

#ifdef __SWITCH__
        consoleUpdate(nullptr);
#endif
    }

    return 0;
}
