#include "test_common.hpp"
#include <nxdev/input.hpp>
#include <nxdev/filesystem.hpp>
#include <nxdev/account.hpp>
#include <nxdev/network.hpp>
#include <nxdev/time.hpp>
#include <nxdev/power.hpp>

#include <iostream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

using nxdev::u32;
using nxdev::u64;

int main() {
    std::cout << "[Test] Running NXDevSDK Platform Modules Test Suite...\n";

    // =========================================================================
    // 1. Input Module Tests
    // =========================================================================
    std::cout << "  -> Testing Input module...\n";
    {
        using namespace nxdev::input;

        // Button operators
        Button ab = Button::A | Button::B;
        NXDEV_TEST_ASSERT((ab & Button::A) == Button::A);
        NXDEV_TEST_ASSERT((ab & Button::B) == Button::B);
        NXDEV_TEST_ASSERT((ab & Button::X) == Button::None);

        Button combined = Button::None;
        combined |= Button::Plus;
        combined |= Button::Minus;
        NXDEV_TEST_ASSERT(combined == (Button::Plus | Button::Minus));

        // Controller simulation
        Controller controller(Player::One);
        NXDEV_TEST_ASSERT(controller.player() == Player::One);

        // Frame 1: Press A and X
        controller.simulate_state(static_cast<u64>(Button::A | Button::X), 16384, -16384, 0, 0);
        controller.update();

        NXDEV_TEST_ASSERT(controller.is_connected());
        NXDEV_TEST_ASSERT(controller.down(Button::A));
        NXDEV_TEST_ASSERT(controller.down(Button::X));
        NXDEV_TEST_ASSERT(!controller.down(Button::B));
        NXDEV_TEST_ASSERT(controller.pressed(Button::A));
        NXDEV_TEST_ASSERT(controller.pressed(Button::X));
        NXDEV_TEST_ASSERT(!controller.pressed(Button::B));
        NXDEV_TEST_ASSERT(!controller.released(Button::A));

        // Stick position
        auto ls = controller.left_stick();
        NXDEV_TEST_ASSERT(ls.raw_x == 16384);
        NXDEV_TEST_ASSERT(ls.raw_y == -16384);
        NXDEV_TEST_ASSERT(ls.x > 0.49f && ls.x < 0.51f);
        NXDEV_TEST_ASSERT(ls.y > -0.51f && ls.y < -0.49f);

        // Deadzone application
        auto dead_stick = ls.apply_deadzone(0.2f);
        NXDEV_TEST_ASSERT(dead_stick.x > 0.0f);
        NXDEV_TEST_ASSERT(dead_stick.y < 0.0f);

        auto small_stick = Stick{0.05f, 0.05f, 1600, 1600}.apply_deadzone(0.15f);
        NXDEV_TEST_ASSERT(small_stick.x == 0.0f);
        NXDEV_TEST_ASSERT(small_stick.y == 0.0f);

        // Frame 2: Release A, hold X, press B
        controller.simulate_state(static_cast<u64>(Button::X | Button::B));
        controller.update();

        NXDEV_TEST_ASSERT(controller.down(Button::X));
        NXDEV_TEST_ASSERT(controller.down(Button::B));
        NXDEV_TEST_ASSERT(!controller.down(Button::A));
        NXDEV_TEST_ASSERT(!controller.pressed(Button::X)); // Held, not newly pressed
        NXDEV_TEST_ASSERT(controller.pressed(Button::B));  // Newly pressed
        NXDEV_TEST_ASSERT(controller.released(Button::A)); // Newly released

        // Style strings
        NXDEV_TEST_ASSERT(controller_style_to_string(ControllerStyle::ProController) == "ProController");
        NXDEV_TEST_ASSERT(controller_style_to_string(ControllerStyle::Handheld) == "Handheld");

        // Touch list
        auto touch_list = touches();
        (void)touch_list;
    }
    std::cout << "  ✓ Input module passed\n";

    // =========================================================================
    // 2. Filesystem Module Tests
    // =========================================================================
    std::cout << "  -> Testing Filesystem module...\n";
    {
        using namespace nxdev::filesystem;

        // Path helpers
        NXDEV_TEST_ASSERT(romfs_path("config.json") == "romfs:/config.json");
        NXDEV_TEST_ASSERT(romfs_path("/assets/font.ttf") == "romfs:/assets/font.ttf");
        NXDEV_TEST_ASSERT(sd_path("switch/myapp/save.dat") == "sdmc:/switch/myapp/save.dat");
        NXDEV_TEST_ASSERT(app_data_path("0100000000002001", "settings.ini") == "sdmc:/switch/0100000000002001/settings.ini");

        // RomFS mount RAII
        {
            auto romfs = RomFS::mount();
            NXDEV_TEST_ASSERT(romfs.is_success());
            NXDEV_TEST_ASSERT(romfs->is_mounted());
            NXDEV_TEST_ASSERT(romfs->mount_name() == "romfs");
            NXDEV_TEST_ASSERT(static_cast<bool>(*romfs));

            // Move semantics
            RomFS moved = std::move(romfs.value());
            NXDEV_TEST_ASSERT(moved.is_mounted());
            NXDEV_TEST_ASSERT(!romfs->is_mounted());
        }

        // File operations using temporary directory
        fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_fs_dir";
        auto mkdir_res = create_directories(temp_dir.string());
        NXDEV_TEST_ASSERT(mkdir_res.is_success());

        fs::path file_txt = temp_dir / "test with spaces.txt";
        std::string test_data = "NXDev C++20 Homebrew Platform Module Test Content\nLine 2";

        // Write text
        auto write_res = write_text(file_txt.string(), test_data);
        NXDEV_TEST_ASSERT(write_res.is_success());
        NXDEV_TEST_ASSERT(exists(file_txt.string()));
        NXDEV_TEST_ASSERT(is_file(file_txt.string()));
        NXDEV_TEST_ASSERT(!is_directory(file_txt.string()));

        // File size
        auto size_res = file_size(file_txt.string());
        NXDEV_TEST_ASSERT(size_res.is_success());
        NXDEV_TEST_ASSERT(size_res.value() == test_data.size());

        // Read text
        auto read_res = read_text(file_txt.string());
        NXDEV_TEST_ASSERT(read_res.is_success());
        NXDEV_TEST_ASSERT(read_res.value() == test_data);

        // Read binary
        auto bytes_res = read_bytes(file_txt.string());
        NXDEV_TEST_ASSERT(bytes_res.is_success());
        NXDEV_TEST_ASSERT(bytes_res->size() == test_data.size());

        // Max size safety check
        auto small_read_res = read_text(file_txt.string(), 10);
        NXDEV_TEST_ASSERT(small_read_res.is_failure());
        NXDEV_TEST_ASSERT(small_read_res.code() == results::FileTooLarge);

        // Nonexistent file
        auto non_exist = read_text(temp_dir.string() + "/does_not_exist.txt");
        NXDEV_TEST_ASSERT(non_exist.is_failure());
        NXDEV_TEST_ASSERT(non_exist.code() == results::FileNotFound);

        // Empty file handling
        fs::path empty_txt = temp_dir / "empty.txt";
        auto empty_write = write_text(empty_txt.string(), "");
        NXDEV_TEST_ASSERT(empty_write.is_success());
        auto empty_read = read_text(empty_txt.string());
        NXDEV_TEST_ASSERT(empty_read.is_success());
        NXDEV_TEST_ASSERT(empty_read.value().empty());

        // Cleanup
        (void)remove(file_txt.string());
        (void)remove(empty_txt.string());
        (void)remove(temp_dir.string());
    }
    std::cout << "  ✓ Filesystem module passed\n";

    // =========================================================================
    // 3. Account Module Tests
    // =========================================================================
    std::cout << "  -> Testing Account module...\n";
    {
        using namespace nxdev::account;

        UserId empty_id;
        NXDEV_TEST_ASSERT(!empty_id.is_valid());
        NXDEV_TEST_ASSERT(!static_cast<bool>(empty_id));

        UserId valid_id(0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL);
        NXDEV_TEST_ASSERT(valid_id.is_valid());
        NXDEV_TEST_ASSERT(static_cast<bool>(valid_id));

        // Hex string serialization
        std::string hex = valid_id.to_string();
        NXDEV_TEST_ASSERT(hex.size() == 32);
        UserId parsed = UserId::from_string(hex);
        NXDEV_TEST_ASSERT(parsed == valid_id);

        // AccountService
        auto svc_res = AccountService::create();
        NXDEV_TEST_ASSERT(svc_res.is_success());
        auto& svc = svc_res.value();

        // Custom simulated profiles
        UserId id1(1, 100);
        UserId id2(2, 200);
        svc.simulate_users({
            Profile{.user_id = id1, .nickname = "Alice"},
            Profile{.user_id = id2, .nickname = "Bob"}
        });

        auto uids = svc.users();
        NXDEV_TEST_ASSERT(uids.is_success());
        NXDEV_TEST_ASSERT(uids->size() == 2);
        NXDEV_TEST_ASSERT((*uids)[0] == id1);
        NXDEV_TEST_ASSERT((*uids)[1] == id2);

        auto count = svc.user_count();
        NXDEV_TEST_ASSERT(count.is_success());
        NXDEV_TEST_ASSERT(count.value() == 2);

        auto prof1 = svc.get_profile(id1);
        NXDEV_TEST_ASSERT(prof1.is_success());
        NXDEV_TEST_ASSERT(prof1->nickname == "Alice");

        auto prof2 = svc.get_profile(id2);
        NXDEV_TEST_ASSERT(prof2.is_success());
        NXDEV_TEST_ASSERT(prof2->nickname == "Bob");

        auto non_exist_prof = svc.get_profile(UserId(999, 999));
        NXDEV_TEST_ASSERT(non_exist_prof.is_failure());
    }
    std::cout << "  ✓ Account module passed\n";

    // =========================================================================
    // 4. Network Module Tests
    // =========================================================================
    std::cout << "  -> Testing Network module...\n";
    {
        using namespace nxdev::network;

        SocketOptions opts;
        NXDEV_TEST_ASSERT(opts.is_valid());

        SocketOptions invalid_opts = opts;
        invalid_opts.tcp_tx_buf_size = 0;
        NXDEV_TEST_ASSERT(!invalid_opts.is_valid());

        // SocketService RAII
        {
            auto sock_res = SocketService::create_default();
            NXDEV_TEST_ASSERT(sock_res.is_success());
            NXDEV_TEST_ASSERT(sock_res->is_active());
            NXDEV_TEST_ASSERT(static_cast<bool>(*sock_res));

            // Reference counting with second service
            auto sock_res2 = SocketService::create_default();
            NXDEV_TEST_ASSERT(sock_res2.is_success());
            NXDEV_TEST_ASSERT(sock_res2->is_active());

            // Move semantics
            SocketService moved = std::move(sock_res2.value());
            NXDEV_TEST_ASSERT(moved.is_active());
            NXDEV_TEST_ASSERT(!sock_res2->is_active());
        }

        // Network status queries
        NXDEV_TEST_ASSERT(is_connected());
        auto ip = get_local_ip();
        NXDEV_TEST_ASSERT(ip.is_success());
        NXDEV_TEST_ASSERT(!ip->empty());
    }
    std::cout << "  ✓ Network module passed\n";

    // =========================================================================
    // 5. Time Module Tests
    // =========================================================================
    std::cout << "  -> Testing Time module...\n";
    {
        using namespace nxdev::time;

        // Clock monotonic ordering
        auto t0 = Clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        auto t1 = Clock::now();
        NXDEV_TEST_ASSERT(t1 >= t0);

        auto mono_ns = monotonic_time();
        auto mono_ms = monotonic_time_ms();
        NXDEV_TEST_ASSERT(mono_ns.count() > 0);
        NXDEV_TEST_ASSERT(mono_ms.count() > 0);

        // System tick conversions
        u64 freq = get_system_tick_frequency();
        NXDEV_TEST_ASSERT(freq > 0);

        u64 tick = get_system_tick();
        NXDEV_TEST_ASSERT(tick > 0);

        u64 ns_val = 1000000ULL; // 1 ms
        u64 ticks = nanoseconds_to_ticks(ns_val);
        u64 roundtrip_ns = ticks_to_nanoseconds(ticks);
        // Small integer division tolerance
        NXDEV_TEST_ASSERT(roundtrip_ns >= ns_val - 100 && roundtrip_ns <= ns_val + 100);

        // DateTime format
        DateTime dt = wall_clock_now();
        NXDEV_TEST_ASSERT(dt.year >= 2024);
        NXDEV_TEST_ASSERT(dt.month >= 1 && dt.month <= 12);
        NXDEV_TEST_ASSERT(dt.day >= 1 && dt.day <= 31);
        std::string iso = dt.format_iso8601();
        NXDEV_TEST_ASSERT(iso.size() == 20); // YYYY-MM-DDTHH:MM:SSZ
        NXDEV_TEST_ASSERT(iso.back() == 'Z');

        // sleep_for helper
        sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "  ✓ Time module passed\n";

    // =========================================================================
    // 6. Power Module Tests
    // =========================================================================
    std::cout << "  -> Testing Power module...\n";
    {
        using namespace nxdev::power;

        auto op = get_operation_mode();
        NXDEV_TEST_ASSERT(op == OperationMode::Handheld || op == OperationMode::Docked || op == OperationMode::Unknown);
        NXDEV_TEST_ASSERT(!operation_mode_to_string(op).empty());

        auto focus = get_focus_state();
        NXDEV_TEST_ASSERT(!focus_state_to_string(focus).empty());

        auto perf = get_performance_mode();
        NXDEV_TEST_ASSERT(!performance_mode_to_string(perf).empty());

        auto bat = get_battery_info();
        NXDEV_TEST_ASSERT(bat.is_success());
        NXDEV_TEST_ASSERT(bat->percentage <= 100);

        // Event hook registration RAII
        bool hook_called = false;
        {
            auto hook = register_event_handler([&](AppletEvent ev) {
                (void)ev;
                hook_called = true;
            });
            NXDEV_TEST_ASSERT(hook.is_active());
            NXDEV_TEST_ASSERT(static_cast<bool>(hook));

            // Move semantics
            EventHookRegistration moved = std::move(hook);
            NXDEV_TEST_ASSERT(moved.is_active());
            NXDEV_TEST_ASSERT(!hook.is_active());
        }
        NXDEV_TEST_ASSERT(!hook_called); // Hook destroyed cleanly without calling
    }
    std::cout << "  ✓ Power module passed\n";

    std::cout << "[Test] All NXDevSDK Platform Modules tests passed successfully!\n";
    return 0;
}
