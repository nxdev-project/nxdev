#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int NewCommand::execute([[maybe_unused]] std::span<const std::string> args) {
    std::cout << "[nxdev new] Notice: Project scaffolding is currently in development.\n";
    return 0;
}

} // namespace nxdev::cli
