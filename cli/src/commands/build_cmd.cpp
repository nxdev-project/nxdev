#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int BuildCommand::execute([[maybe_unused]] std::span<const std::string> args) {
    std::cout << "[nxdev build] Notice: Project compilation orchestration is currently in development.\n";
    return 0;
}

} // namespace nxdev::cli
