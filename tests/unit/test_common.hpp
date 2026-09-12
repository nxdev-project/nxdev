#pragma once

#include <iostream>
#include <cstdlib>

#define NXDEV_TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "TEST ASSERTION FAILED: " #condition \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::abort(); \
        } \
    } while (false)
