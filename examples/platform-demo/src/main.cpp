#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <nxdev/filesystem.hpp>
#include <nxdev/account.hpp>
#include <nxdev/network.hpp>
#include <nxdev/time.hpp>
#include <nxdev/power.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Initializing NXDev Platform Demo...");

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

    printf("\x1b[2;2H=== NXDevSDK Platform Demo ===");
#endif

    // 1. Filesystem & RomFS
    std::string romfs_msg = "(RomFS not mounted)";
    auto romfs_res = nxdev::filesystem::RomFS::mount();
    if (romfs_res.is_success()) {
        auto text_res = nxdev::filesystem::read_text("romfs:/greeting.txt");
        if (text_res.is_success()) {
            romfs_msg = text_res.value();
            // Trim newline if present
            if (!romfs_msg.empty() && romfs_msg.back() == '\n') {
                romfs_msg.pop_back();
            }
        } else {
            romfs_msg = "Mounted, but greeting.txt not found";
        }
    }

    // 2. Input / Controller
    nxdev::input::Controller controller(nxdev::input::Player::Auto);

    // 3. User / Account
    std::string user_name = "Guest User";
    auto user_res = nxdev::account::get_last_opened_user();
    if (user_res.is_success()) {
        auto prof_res = nxdev::account::get_profile(user_res.value());
        if (prof_res.is_success()) {
            user_name = prof_res->nickname;
        }
    }

    // 4. Network / Sockets
    std::string local_ip = "Offline / Unknown";
    auto net_res = nxdev::network::SocketService::create_default();
    if (net_res.is_success()) {
        auto ip_res = nxdev::network::get_local_ip();
        if (ip_res.is_success()) {
            local_ip = ip_res.value();
        }
    }

    // 5. Power & Operation Mode
    auto op_mode = nxdev::power::get_operation_mode();
    auto focus_state = nxdev::power::get_focus_state();
    auto battery = nxdev::power::get_battery_info().value_or(nxdev::power::BatteryInfo{100, false});

    // 6. Time & Calendar
    auto dt = nxdev::time::wall_clock_now();
    nxdev::u64 sys_tick = nxdev::time::get_system_tick();

#ifdef __SWITCH__
    printf("\x1b[4;2HUser: %s", user_name.c_str());
    printf("\x1b[5;2HNetwork IP: %s", local_ip.c_str());
    printf("\x1b[6;2HOperation Mode: %.*s | Focus: %.*s | Battery: %u%%",
           static_cast<int>(nxdev::power::operation_mode_to_string(op_mode).size()),
           nxdev::power::operation_mode_to_string(op_mode).data(),
           static_cast<int>(nxdev::power::focus_state_to_string(focus_state).size()),
           nxdev::power::focus_state_to_string(focus_state).data(),
           battery.percentage);
    printf("\x1b[7;2HDate/Time: %s (SysTick: %llu)", dt.format_iso8601().c_str(), (unsigned long long)sys_tick);
    printf("\x1b[9;2HRomFS Status: %s", romfs_msg.c_str());
    printf("\x1b[12;2HPress + (Plus) to exit.");
#else
    std::cout << "User: " << user_name << "\n";
    std::cout << "Network IP: " << local_ip << "\n";
    std::cout << "Operation Mode: " << nxdev::power::operation_mode_to_string(op_mode) << "\n";
    std::cout << "Focus State: " << nxdev::power::focus_state_to_string(focus_state) << "\n";
    std::cout << "Battery: " << battery.percentage << "%\n";
    std::cout << "Date/Time: " << dt.format_iso8601() << " (SysTick: " << sys_tick << ")\n";
    std::cout << "RomFS: " << romfs_msg << "\n";
#endif

    nxdev::log::info("Entering Platform Demo main loop...");

    while (app.running()) {
        controller.update();

        if (controller.pressed(nxdev::input::Button::Plus)) {
            nxdev::log::info("Plus button pressed. Exiting...");
            app.request_exit();
            break;
        }

        if (controller.pressed(nxdev::input::Button::A)) {
            nxdev::log::info("A button pressed.");
        }

#ifdef __SWITCH__
        consoleUpdate(nullptr);
#else
        // Single host tick simulation
        app.request_exit();
#endif
    }

    nxdev::log::info("Platform Demo finished successfully.");
    return 0;
}
