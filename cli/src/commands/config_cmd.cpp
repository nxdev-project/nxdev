#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int ConfigCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (args.empty()) {
        std::cout << "Usage: " << usage() << "\n\n";
        std::cout << "Subcommands:\n";
        std::cout << "  path                Display configuration file locations\n";
        std::cout << "  list                List all active configuration key-value pairs\n";
        std::cout << "  get <key>           Get value for specific configuration key\n";
        return 0;
    }

    std::string sub = args[0];

    if (sub == "path") {
        std::cout << "Global Config Path: " << ctx.config.global_config_path()
                  << " (" << (ctx.config.has_global_config() ? "exists" : "not found") << ")\n";
        if (ctx.project.has_value()) {
            std::cout << "Local Config Path:  " << ctx.config.local_config_path()
                      << " (" << (ctx.config.has_local_config() ? "exists" : "not found") << ")\n";
        }
        return 0;
    }

    if (sub == "list") {
        auto entries = ctx.config.list_entries();
        if (entries.empty()) {
            std::cout << "(No custom configuration entries set)\n";
        } else {
            std::cout << "=== NXDev Configuration ===\n";
            for (const auto& [k, v] : entries) {
                std::cout << "  " << k << " = " << v << "\n";
            }
        }
        return 0;
    }

    if (sub == "get") {
        if (args.size() < 2) {
            std::cerr << "Error: 'config get' requires a <key> argument.\n";
            return 1;
        }
        std::string key = args[1];
        auto val = ctx.config.get_value(key);
        if (val.has_value()) {
            std::cout << *val << "\n";
            return 0;
        } else {
            std::cerr << "Configuration key '" << key << "' is not set.\n";
            return 1;
        }
    }

    std::cerr << "Unknown config subcommand '" << sub << "'. Run 'nxdev config' for help.\n";
    return 1;
}

} // namespace nxdev::cli
