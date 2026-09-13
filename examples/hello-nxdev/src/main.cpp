#include <nxdev/nxdev.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Initializing Hello NXDev application...");

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

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[2;2H=== Hello NXDev (NXDevSDK Core) ===");
    printf("\x1b[4;2HSDK Version: %.*s", static_cast<int>(nxdev::version().string.size()), nxdev::version().string.data());

    auto fw = nxdev::system::get_firmware_version();
    printf("\x1b[5;2HFirmware: %d.%d.%d (%s)", fw.major, fw.minor, fw.micro, fw.display_version.c_str());

    auto applet_type = nxdev::system::get_applet_type();
    printf("\x1b[6;2HApplet Type: %.*s", static_cast<int>(nxdev::applet_type_to_string(applet_type).size()), nxdev::applet_type_to_string(applet_type).data());

    u64 total_mem = nxdev::system::get_total_memory();
    u64 used_mem = nxdev::system::get_used_memory();
    printf("\x1b[7;2HMemory: %llu MB / %llu MB", (unsigned long long)(used_mem / (1024 * 1024)), (unsigned long long)(total_mem / (1024 * 1024)));

    printf("\x1b[9;2HLogs are dispatched via nxdev::log subsystem.");
    printf("\x1b[11;2HPress + (Plus) to exit.");
#else
    nxdev::log::info("Running in Host / Mock mode.");
    auto fw = nxdev::system::get_firmware_version();
    std::cout << "NXDev SDK Version: " << nxdev::version().string << "\n";
    std::cout << "Firmware (Mock): " << fw.display_version << "\n";
    std::cout << "Applet Type: " << nxdev::applet_type_to_string(app.applet_type()) << "\n";
#endif

    nxdev::log::info("Entering main application loop...");

    while (app.running()) {
#ifdef __SWITCH__
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus) {
            nxdev::log::info("Plus button pressed. Requesting exit...");
            app.request_exit();
            break;
        }
        consoleUpdate(nullptr);
#else
        // Host simulation: single tick
        nxdev::log::info("Simulated host loop tick executed successfully.");
        app.request_exit();
#endif
    }

    nxdev::log::info("Exiting Hello NXDev gracefully.");
    return 0;
}
