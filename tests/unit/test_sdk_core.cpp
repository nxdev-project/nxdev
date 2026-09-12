#include "test_common.hpp"
#include <nxdev/nxdev.hpp>
#include <nxdev/input.hpp>
#include <nxdev/display.hpp>
#include <iostream>

int main() {
    std::cout << "[Test] Running SDK Core tests...\n";

    // Test Result abstractions
    nxdev::Result ok = nxdev::results::Success;
    NXDEV_TEST_ASSERT(ok.is_success());
    NXDEV_TEST_ASSERT(!ok.is_failure());
    NXDEV_TEST_ASSERT(static_cast<bool>(ok));
    NXDEV_TEST_ASSERT(ok.get_error_name() == "Success");

    nxdev::Result err = nxdev::results::NotInitialized;
    NXDEV_TEST_ASSERT(err.is_failure());
    NXDEV_TEST_ASSERT(!err.is_success());
    NXDEV_TEST_ASSERT(!static_cast<bool>(err));
    NXDEV_TEST_ASSERT(err.get_error_name() == "NotInitialized");

    // Test App lifecycle
    nxdev::App app;
    NXDEV_TEST_ASSERT(!app.is_running());

    nxdev::Result init_res = app.initialize();
    NXDEV_TEST_ASSERT(init_res.is_success());
    NXDEV_TEST_ASSERT(app.is_running());

    // Duplicate initialization should fail with AlreadyInitialized
    nxdev::Result dup_res = app.initialize();
    NXDEV_TEST_ASSERT(dup_res.is_failure());
    NXDEV_TEST_ASSERT(dup_res == nxdev::results::AlreadyInitialized);

    app.request_exit();
    NXDEV_TEST_ASSERT(!app.is_running());

    app.finalize();
    NXDEV_TEST_ASSERT(!app.is_running());

    // Test Input module
    nxdev::input::InputManager input;
    nxdev::Result in_res = input.initialize();
    NXDEV_TEST_ASSERT(in_res.is_success());
    NXDEV_TEST_ASSERT(!input.is_down(nxdev::input::Button::A));
    input.finalize();

    // Test Display module
    nxdev::display::DisplayManager display;
    nxdev::Result disp_res = display.initialize();
    NXDEV_TEST_ASSERT(disp_res.is_success());
    auto res = display.get_resolution();
    NXDEV_TEST_ASSERT(res.width == 1280);
    NXDEV_TEST_ASSERT(res.height == 720);
    display.finalize();

    std::cout << "[Test] SDK Core tests passed successfully.\n";
    return 0;
}
