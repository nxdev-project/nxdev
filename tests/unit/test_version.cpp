#include "test_common.hpp"
#include <nxdev/types.hpp>
#include <iostream>
#include <string>

int main() {
    std::cout << "[Test] Running Version tests...\n";

    NXDEV_TEST_ASSERT(nxdev::CURRENT_VERSION.major == 0);
    NXDEV_TEST_ASSERT(nxdev::CURRENT_VERSION.minor == 1);
    NXDEV_TEST_ASSERT(nxdev::CURRENT_VERSION.patch == 0);
    NXDEV_TEST_ASSERT(nxdev::CURRENT_VERSION.prerelease == "beta.1");

#ifdef NXDEV_VERSION
    std::string ver = NXDEV_VERSION;
    NXDEV_TEST_ASSERT(!ver.empty());
    NXDEV_TEST_ASSERT(ver == "0.1.0-beta.1");
#endif

    std::cout << "[Test] Version tests passed successfully.\n";
    return 0;
}
