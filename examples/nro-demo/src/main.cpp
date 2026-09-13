#include <nxdev/nxdev.hpp>
#include <nxdev/filesystem.hpp>
#include <nxdev/input.hpp>
#include <nxdev/time.hpp>
#include <iostream>

#ifdef __SWITCH__
#include <switch.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    auto app_res = nxdev::App::create();
    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize NXDev App!");
        return 1;
    }
    auto& app = app_res.value();

#ifdef __SWITCH__
    consoleInit(nullptr);
    auto console_guard = nxdev::scope_exit([]() {
        consoleExit(nullptr);
    });
#endif

    std::cout << "=== NXDevPack NRO Packaging Demo ===\n\n";
    std::cout << "Application: NRO Packaging Demo v1.0.0\n";
    std::cout << "Features:    Custom JPEG Icon + RomFS Assets\n\n";

    // Mount RomFS and read text asset
    auto romfs_res = nxdev::filesystem::RomFS::mount();
    if (romfs_res.is_success()) {
        auto greeting = nxdev::filesystem::read_text("romfs:/greeting.txt");
        if (greeting.is_success()) {
            std::cout << "RomFS content: " << greeting.value() << "\n\n";
        } else {
            std::cout << "Notice: Could not read romfs:/greeting.txt (" << greeting.error_name() << ")\n\n";
        }
    } else {
        std::cout << "Notice: RomFS not mounted (" << romfs_res.error_name() << ")\n\n";
    }

    std::cout << "Press PLUS (+) to exit...\n";

    nxdev::input::Controller controller(nxdev::input::Player::Auto);

    while (app.is_running()) {
        controller.update();
        if (controller.down(nxdev::input::Button::Plus)) {
            break;
        }
#ifdef __SWITCH__
        consoleUpdate(nullptr);
#endif
        nxdev::time::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}

