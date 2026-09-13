#include "test_common.hpp"
#include <nxdev/nxdev.hpp>
#include <iostream>
#include <string>
#include <vector>

using nxdev::u32;
using nxdev::u64;

// Helper function to test NXDEV_TRY macro
static nxdev::Result<void> helper_void_op(bool fail) {
    if (fail) {
        return nxdev::results::InvalidArgument;
    }
    return nxdev::Result<void>::Success();
}

static nxdev::Result<std::string> test_try_propagation(bool fail) {
    NXDEV_TRY(helper_void_op(fail));
    return nxdev::Result<std::string>("Success: 42");
}

int main() {
    std::cout << "[Test] Running Comprehensive SDK Core tests...\n";

    // ==========================================
    // 1. Result<void> Tests
    // ==========================================
    {
        nxdev::Result<void> ok = nxdev::results::Success;
        NXDEV_TEST_ASSERT(ok.is_success());
        NXDEV_TEST_ASSERT(ok.succeeded());
        NXDEV_TEST_ASSERT(!ok.is_failure());
        NXDEV_TEST_ASSERT(!ok.failed());
        NXDEV_TEST_ASSERT(static_cast<bool>(ok));
        NXDEV_TEST_ASSERT(ok.raw() == 0);
        NXDEV_TEST_ASSERT(ok.module() == 0);
        NXDEV_TEST_ASSERT(ok.description() == 0);
        NXDEV_TEST_ASSERT(ok.error_name() == "Success");

        // Module: 16 (OS), Description: 100
        u32 raw_err = (16 & 0x1FF) | ((100 & 0x1FFF) << 9);
        nxdev::Result<void> err(raw_err);
        NXDEV_TEST_ASSERT(err.is_failure());
        NXDEV_TEST_ASSERT(err.failed());
        NXDEV_TEST_ASSERT(!err.is_success());
        NXDEV_TEST_ASSERT(!err.succeeded());
        NXDEV_TEST_ASSERT(!static_cast<bool>(err));
        NXDEV_TEST_ASSERT(err.raw() == raw_err);
        NXDEV_TEST_ASSERT(err.native_result() == raw_err);
        NXDEV_TEST_ASSERT(err.module() == 16);
        NXDEV_TEST_ASSERT(err.description() == 100);
        NXDEV_TEST_ASSERT(err.error_name() == "SplError");

        // from_libnx helper
        auto libnx_ok = nxdev::from_libnx(0);
        NXDEV_TEST_ASSERT(libnx_ok.is_success());
        auto libnx_err = nxdev::from_libnx(0x202); // Module 2 (FS)
        NXDEV_TEST_ASSERT(libnx_err.is_failure());
        NXDEV_TEST_ASSERT(libnx_err.module() == 2);
    }

    // ==========================================
    // 2. Result<T> Tests
    // ==========================================
    {
        nxdev::Result<int> val_res(123);
        NXDEV_TEST_ASSERT(val_res.is_success());
        NXDEV_TEST_ASSERT(val_res.value() == 123);
        NXDEV_TEST_ASSERT(*val_res == 123);
        NXDEV_TEST_ASSERT(val_res.value_or(999) == 123);

        struct TestStruct {
            std::string name;
            int id;
        };

        nxdev::Result<TestStruct> struct_res(TestStruct{"switch", 7});
        NXDEV_TEST_ASSERT(struct_res.is_success());
        NXDEV_TEST_ASSERT(struct_res->name == "switch");
        NXDEV_TEST_ASSERT(struct_res->id == 7);

        nxdev::Result<int> fail_res(nxdev::results::NotFound);
        NXDEV_TEST_ASSERT(fail_res.is_failure());
        NXDEV_TEST_ASSERT(fail_res.raw() == nxdev::results::NotFound.raw());
        NXDEV_TEST_ASSERT(fail_res.value_or(999) == 999);

        // from_libnx with typed value
        auto typed_ok = nxdev::from_libnx(0, std::string("horizon"));
        NXDEV_TEST_ASSERT(typed_ok.is_success());
        NXDEV_TEST_ASSERT(typed_ok.value() == "horizon");

        auto typed_err = nxdev::from_libnx(0x100, std::string("horizon"));
        NXDEV_TEST_ASSERT(typed_err.is_failure());
        NXDEV_TEST_ASSERT(typed_err.value_or("default") == "default");

        // NXDEV_TRY macro test
        auto try_ok = test_try_propagation(false);
        NXDEV_TEST_ASSERT(try_ok.is_success());
        NXDEV_TEST_ASSERT(try_ok.value() == "Success: 42");

        auto try_fail = test_try_propagation(true);
        NXDEV_TEST_ASSERT(try_fail.is_failure());
        NXDEV_TEST_ASSERT(try_fail.code() == nxdev::results::InvalidArgument);
    }

    // ==========================================
    // 3. ScopeExit Tests
    // ==========================================
    {
        bool executed = false;
        {
            auto guard = nxdev::scope_exit([&]() {
                executed = true;
            });
            NXDEV_TEST_ASSERT(guard.is_active());
            NXDEV_TEST_ASSERT(!executed);
        }
        NXDEV_TEST_ASSERT(executed);

        // Cancellation test
        bool cancelled_executed = false;
        {
            auto guard = nxdev::scope_exit([&]() {
                cancelled_executed = true;
            });
            guard.cancel();
            NXDEV_TEST_ASSERT(!guard.is_active());
        }
        NXDEV_TEST_ASSERT(!cancelled_executed);

        // Release test
        bool release_executed = false;
        {
            auto guard = nxdev::make_scope_exit([&]() {
                release_executed = true;
            });
            guard.release();
            NXDEV_TEST_ASSERT(!guard.is_active());
        }
        NXDEV_TEST_ASSERT(!release_executed);

        // Move semantics test
        int move_count = 0;
        {
            auto guard1 = nxdev::scope_exit([&]() {
                move_count++;
            });
            auto guard2 = std::move(guard1);
            NXDEV_TEST_ASSERT(!guard1.is_active());
            NXDEV_TEST_ASSERT(guard2.is_active());
        }
        NXDEV_TEST_ASSERT(move_count == 1);
    }

    // ==========================================
    // 4. ServiceGuard Tests
    // ==========================================
    {
        bool service_cleaned = false;
        {
            auto guard = nxdev::ServiceGuard([&]() {
                service_cleaned = true;
            });
            NXDEV_TEST_ASSERT(guard.is_active());
            NXDEV_TEST_ASSERT(guard.is_initialized());
            NXDEV_TEST_ASSERT(static_cast<bool>(guard));
        }
        NXDEV_TEST_ASSERT(service_cleaned);

        // make_service_guard success
        bool init_called = false;
        bool exit_called = false;
        {
            auto res = nxdev::make_service_guard(
                [&]() -> u32 { init_called = true; return 0; },
                [&]() { exit_called = true; }
            );
            NXDEV_TEST_ASSERT(res.is_success());
            NXDEV_TEST_ASSERT(init_called);
            NXDEV_TEST_ASSERT(!exit_called);
        }
        NXDEV_TEST_ASSERT(exit_called);

        // make_service_guard failure
        bool fail_exit_called = false;
        {
            auto res = nxdev::make_service_guard(
                [&]() -> u32 { return 0x1234; },
                [&]() { fail_exit_called = true; }
            );
            NXDEV_TEST_ASSERT(res.is_failure());
            NXDEV_TEST_ASSERT(res.raw() == 0x1234);
        }
        NXDEV_TEST_ASSERT(!fail_exit_called);
    }

    // ==========================================
    // 5. App Lifecycle Tests
    // ==========================================
    {
        auto app_res = nxdev::App::create();
        NXDEV_TEST_ASSERT(app_res.is_success());
        auto& app = app_res.value();
        NXDEV_TEST_ASSERT(app.running());
        NXDEV_TEST_ASSERT(!app.exit_requested());

        // Duplicate initialize should fail
        auto dup_res = app.initialize();
        NXDEV_TEST_ASSERT(dup_res.is_failure());
        NXDEV_TEST_ASSERT(dup_res.code() == nxdev::results::AlreadyInitialized);

        // Applet type
        auto applet_type = app.applet_type();
        NXDEV_TEST_ASSERT(applet_type == nxdev::AppletType::Application);
        NXDEV_TEST_ASSERT(nxdev::applet_type_to_string(applet_type) == "Application");

        // Exit request
        app.request_exit();
        NXDEV_TEST_ASSERT(app.exit_requested());
        NXDEV_TEST_ASSERT(!app.running());

        app.finalize();
        NXDEV_TEST_ASSERT(!app.running());
    }

    // ==========================================
    // 6. Logging Subsystem Tests
    // ==========================================
    {
        std::vector<std::pair<nxdev::log::Level, std::string>> captured_logs;
        nxdev::log::set_custom_sink([&](nxdev::log::Level lvl, std::string_view msg) {
            captured_logs.emplace_back(lvl, std::string(msg));
        });

        nxdev::log::set_level(nxdev::log::Level::Info);
        NXDEV_TEST_ASSERT(nxdev::log::get_level() == nxdev::log::Level::Info);

        nxdev::log::trace("This trace should be ignored");
        nxdev::log::debug("This debug should be ignored");
        nxdev::log::info("This info should be recorded");
        nxdev::log::warn("This warn should be recorded");
        nxdev::log::error("This error should be recorded");

        NXDEV_TEST_ASSERT(captured_logs.size() == 3);
        NXDEV_TEST_ASSERT(captured_logs[0].first == nxdev::log::Level::Info);
        NXDEV_TEST_ASSERT(captured_logs[0].second.find("This info should be recorded") != std::string::npos);
        NXDEV_TEST_ASSERT(captured_logs[1].first == nxdev::log::Level::Warn);
        NXDEV_TEST_ASSERT(captured_logs[1].second.find("This warn should be recorded") != std::string::npos);
        NXDEV_TEST_ASSERT(captured_logs[2].first == nxdev::log::Level::Error);
        NXDEV_TEST_ASSERT(captured_logs[2].second.find("This error should be recorded") != std::string::npos);

        // Level::Off
        captured_logs.clear();
        nxdev::log::set_level(nxdev::log::Level::Off);
        nxdev::log::error("Should be silenced");
        NXDEV_TEST_ASSERT(captured_logs.empty());

        // Level strings
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Trace) == "TRACE");
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Debug) == "DEBUG");
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Info) == "INFO");
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Warn) == "WARN");
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Error) == "ERROR");
        NXDEV_TEST_ASSERT(nxdev::log::level_to_string(nxdev::log::Level::Off) == "OFF");

        nxdev::log::reset_sink();
    }

    // ==========================================
    // 7. System & Version Tests
    // ==========================================
    {
        auto fw = nxdev::system::get_firmware_version();
        NXDEV_TEST_ASSERT(!fw.display_version.empty());

        auto applet_type = nxdev::system::get_applet_type();
        NXDEV_TEST_ASSERT(applet_type != nxdev::AppletType::None);

        u64 total_mem = nxdev::system::get_total_memory();
        u64 used_mem = nxdev::system::get_used_memory();
        NXDEV_TEST_ASSERT(total_mem > 0);
        NXDEV_TEST_ASSERT(used_mem <= total_mem);

        auto ver = nxdev::version();
        NXDEV_TEST_ASSERT(ver.major == NXDEV_VERSION_MAJOR);
        NXDEV_TEST_ASSERT(ver.minor == NXDEV_VERSION_MINOR);
        NXDEV_TEST_ASSERT(ver.patch == NXDEV_VERSION_PATCH);
        NXDEV_TEST_ASSERT(!ver.string.empty());
    }

    std::cout << "[Test] All SDK Core tests passed successfully!\n";
    return 0;
}
