#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int DoctorCommand::execute([[maybe_unused]] std::span<const std::string> args) {
    std::cout << "[nxdev doctor] Environment diagnostic check:\n";
    std::cout << "  (Environment detection and validation will be implemented in a subsequent stage)\n";
    return 0;
}

} // namespace nxdev::cli
