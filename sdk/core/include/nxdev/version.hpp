#pragma once

#include <nxdev/types.hpp>
#include <string_view>

#define NXDEV_VERSION_MAJOR 0
#define NXDEV_VERSION_MINOR 1
#define NXDEV_VERSION_PATCH 0
#define NXDEV_VERSION_STRING "0.1.0-beta.1"

namespace nxdev {

struct SdkVersion {
    u32 major{NXDEV_VERSION_MAJOR};
    u32 minor{NXDEV_VERSION_MINOR};
    u32 patch{NXDEV_VERSION_PATCH};
    std::string_view prerelease{"beta.1"};
    std::string_view string{NXDEV_VERSION_STRING};
};

/**
 * @brief Returns the version of the NXDevSDK.
 */
[[nodiscard]] SdkVersion version() noexcept;

} // namespace nxdev
