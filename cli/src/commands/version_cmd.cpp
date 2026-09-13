#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int VersionCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    bool as_json = false;
    for (const auto& a : args) {
        if (a == "--json") as_json = true;
    }

#ifdef NXDEV_VERSION
    std::string ver = NXDEV_VERSION;
#else
    std::string ver = "0.1.0-dev";
#endif

    if (as_json) {
        std::cout << "{\n";
        std::cout << "  \"version\": \"" << ver << "\",\n";
        std::cout << "  \"host\": \"" << ctx.env.host().os_name() << " (" << ctx.env.host().arch_name() << ")\",\n";
        std::cout << "  \"ecosystem\": \"NXDev Nintendo Switch Development Ecosystem\"\n";
        std::cout << "}\n";
    } else {
        std::cout << "NXDev CLI version " << ver << "\n";
        std::cout << "Host: " << ctx.env.host().os_name() << " (" << ctx.env.host().arch_name() << ")\n";
        std::cout << "Unofficial Nintendo Switch Homebrew Development Ecosystem\n";
    }
    return 0;
}

} // namespace nxdev::cli
