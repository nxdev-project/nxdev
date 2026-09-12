#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int VersionCommand::execute([[maybe_unused]] std::span<const std::string> args) {
#ifdef NXDEV_VERSION
    std::cout << "NXDev CLI version " << NXDEV_VERSION << "\n";
#else
    std::cout << "NXDev CLI version 0.1.0-dev\n";
#endif
    std::cout << "Target: Host (development build)\n";
    std::cout << "Unofficial Nintendo Switch Homebrew Development Ecosystem\n";
    return 0;
}

} // namespace nxdev::cli
