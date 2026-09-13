#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int EnvCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    bool as_json = false;
    for (const auto& a : args) {
        if (a == "--json") as_json = true;
    }

    if (as_json) {
        std::cout << ctx.env.to_json(ctx.project) << "\n";
    } else {
        std::cout << "=== NXDev Host & Toolchain Environment ===\n\n";
        std::cout << ctx.env.summary(ctx.project) << "\n";
    }
    return 0;
}

} // namespace nxdev::cli
