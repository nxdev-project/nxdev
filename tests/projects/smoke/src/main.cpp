#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <nxdev/filesystem.hpp>
#include <iostream>

#ifdef __SWITCH__
#include <switch.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Running NXDev E2E Smoke Application...");

    auto app_res = nxdev::App::create();
    if (app_res.failed()) {
        nxdev::log::error("Smoke: Failed to initialize App");
        return 1;
    }
    auto& app = app_res.value();

    // Verify RomFS mount
    auto romfs_mount = nxdev::filesystem::RomFS::mount();
    if (romfs_mount.is_success()) {
        auto content = nxdev::filesystem::read_text("romfs:/greeting.txt");
        if (content.is_success()) {
            nxdev::log::info("Smoke RomFS payload: " + content.value());
        }
    }

    nxdev::input::Controller controller(nxdev::input::Player::Auto);
    controller.update();

    nxdev::log::info("NXDev Smoke Test Finished successfully.");
    return 0;
}
