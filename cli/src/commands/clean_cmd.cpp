#include <nxdev/cli/commands.hpp>
#include <nxdev/build/build_orchestrator.hpp>
#include <iostream>

namespace nxdev::cli {

int CleanCommand::execute(std::span<const std::string> /*args*/, CommandContext& ctx) {
    if (!ctx.project.has_value()) {
        std::cerr << "error: no NXDev project found.\n"
                  << "hint: execute this command from within a directory containing 'nxapp.yaml' or pass '--project <path>'.\n";
        return 1;
    }

    std::string err;
    bool success = build::BuildOrchestrator::safe_clean(*ctx.project, err);

    if (success) {
        std::cout << "[✓] Cleaned NXDev build directory for project: " << ctx.project->root_path() << "\n";
        return 0;
    } else {
        std::cerr << "[x] Clean failed: " << err << "\n";
        return 1;
    }
}

} // namespace nxdev::cli
