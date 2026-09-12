#include <nxdev/cli/commands.hpp>
#include <nxdev/pack/pack.hpp>
#include <iostream>

namespace nxdev::cli {

int PackCommand::execute([[maybe_unused]] std::span<const std::string> args) {
    std::cout << "[nxdev pack] Notice: Package generation (NRO/NSP) is currently in development.\n";
    return 0;
}

} // namespace nxdev::cli
