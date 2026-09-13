#include <nxdev/nxdev.hpp>
#include <iostream>

#ifdef __SWITCH__
#include <switch.h>
#include <deko3d.hpp>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Starting {{PROJECT_NAME}} (deko3d)...");

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
    std::cout << "\x1b[4;2Hdeko3d Graphics Initialized\n";
    std::cout << "\x1b[6;2HPress PLUS (+) to exit.\n";

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    while (app.is_running() && appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            app.request_exit();
            break;
        }

        consoleUpdate(nullptr);
    }
#else
    while (app.is_running()) {
        break;
    }
#endif

    return 0;
}
