#include <nxdev/cli/commands.hpp>
#include <nxdev/build/build_orchestrator.hpp>
#include <iostream>

namespace nxdev::cli {

int ConfigureCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (!ctx.project.has_value()) {
        std::cerr << "error: no NXDev project found.\n"
                  << "hint: execute this command from within a directory containing 'nxapp.yaml' or pass '--project <path>'.\n";
        return 1;
    }

    build::BuildOptions options;
    bool json_mode = false;

    // Default profile from manifest if available
    if (ctx.project->is_valid()) {
        const auto& b = ctx.project->manifest().build();
        auto p = build::parse_build_profile(b.default_profile);
        if (p.has_value()) {
            options.profile = *p;
        }
    }

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--profile" && i + 1 < args.size()) {
            auto p = build::parse_build_profile(args[++i]);
            if (p.has_value()) {
                options.profile = *p;
            } else {
                std::cerr << "error: invalid build profile '" << args[i] << "'. Supported values: debug, release.\n";
                return 1;
            }
        } else if (args[i] == "--fresh") {
            options.fresh_configure = true;
        } else if (args[i] == "--json") {
            json_mode = true;
        } else if (args[i] == "--verbose" || args[i] == "-v") {
            options.verbose = true;
        }
    }

    if (!json_mode) {
        std::cout << "NXDev Configure\n\n"
                  << "Project: " << (ctx.project->is_valid() ? ctx.project->manifest().application().name : ctx.project->root_path()) << "\n"
                  << "Profile: " << build::build_profile_to_string(options.profile) << "\n\n"
                  << "Running CMake configuration...\n";
    }

    auto res = build::BuildOrchestrator::configure(*ctx.project, ctx.env, options);

    if (json_mode) {
        std::cout << res.to_json() << "\n";
    } else {
        if (!res.stdout_output.empty() && options.verbose) {
            std::cout << res.stdout_output << "\n";
        }
        if (!res.stderr_output.empty() && (!res.success || options.verbose)) {
            std::cerr << res.stderr_output << "\n";
        }

        if (res.success) {
            std::cout << "\n[✓] CMake configuration succeeded!\n"
                      << "Build directory: " << res.build_directory << "\n";
        } else {
            std::cerr << "\n[x] CMake configuration failed (exit code " << res.exit_code << ").\n";
            for (const auto& diag : res.diagnostics) {
                std::cerr << "  - " << diag << "\n";
            }
        }
    }

    return res.success ? 0 : (res.exit_code != 0 ? res.exit_code : 1);
}

} // namespace nxdev::cli
