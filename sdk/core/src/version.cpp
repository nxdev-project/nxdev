#include <nxdev/version.hpp>

namespace nxdev {

SdkVersion version() noexcept {
    return SdkVersion{
        .major = NXDEV_VERSION_MAJOR,
        .minor = NXDEV_VERSION_MINOR,
        .patch = NXDEV_VERSION_PATCH,
        .prerelease = "dev",
        .string = NXDEV_VERSION_STRING
    };
}

} // namespace nxdev
