#include <nxdev/cli/commands.hpp>
#include <iostream>

namespace nxdev::cli {

int ProjectCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (args.empty()) {
        std::cout << "Usage: " << usage() << "\n\n";
        std::cout << "Subcommands:\n";
        std::cout << "  info [--json]       Display active project and manifest metadata\n";
        std::cout << "  root                Print the project root directory path\n";
        return 0;
    }

    std::string sub = args[0];
    bool as_json = false;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--json") as_json = true;
    }

    if (!ctx.project.has_value()) {
        std::cerr << "Error: No NXDev project found.\n";
        std::cerr << "Hint: Run the command inside a project directory containing nxapp.yaml, or pass --project <path>.\n";
        return 1;
    }

    if (sub == "info") {
        if (as_json) {
            std::cout << ctx.project->info_json() << "\n";
        } else {
            std::cout << ctx.project->info_summary() << "\n";
        }
        return 0;
    }

    if (sub == "root") {
        std::cout << ctx.project->root_path() << "\n";
        return 0;
    }

    std::cerr << "Unknown project subcommand '" << sub << "'. Run 'nxdev project' for help.\n";
    return 1;
}

} // namespace nxdev::cli
