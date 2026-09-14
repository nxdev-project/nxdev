#include <nxdev/cli/commands.hpp>
#include <nxdev/build/build_orchestrator.hpp>
#include <nxdev/exec/resource_policy.hpp>
#include <iostream>

namespace nxdev::cli {

int BuildCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
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
        } else if (args[i] == "--clean") {
            options.clean_first = true;
        } else if (args[i] == "--verbose" || args[i] == "-v") {
            options.verbose = true;
        } else if (args[i] == "--jobs" || args[i] == "-j") {
            if (i + 1 < args.size()) {
                options.parallel_jobs = std::stoul(args[++i]);
            }
        } else if (args[i] == "--max-memory" && i + 1 < args.size()) {
            std::string mem_str = args[++i];
            auto parsed = exec::parse_memory_size_string(mem_str);
            if (!parsed.has_value()) {
                std::cerr << "error: invalid --max-memory value '" << mem_str << "'. Examples: 512M, 2G, 4096M, auto, unlimited.\n";
                return 1;
            }
            options.max_memory_bytes = *parsed;
        } else if (args[i] == "--unsafe-no-resource-limits") {
            options.unsafe_no_limits = true;
        } else if (args[i] == "--json") {
            json_mode = true;
        }
    }

    if (!json_mode) {
        std::cout << "NXDev Build\n\n"
                  << "Project: " << (ctx.project->is_valid() ? ctx.project->manifest().application().name : ctx.project->root_path()) << "\n"
                  << "Profile: " << build::build_profile_to_string(options.profile) << "\n\n"
                  << "[1/2] Configure & Verify\n"
                  << "[2/2] Compile & Link (Target: Switch AArch64 ELF)\n\n";
    }

    auto res = build::BuildOrchestrator::build(*ctx.project, ctx.env, options);

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
            std::cout << "\n[✓] Build Succeeded!\n"
                      << "Output ELF: " << res.elf_path << "\n";
            if (!res.compile_commands_path.empty()) {
                std::cout << "Compile commands: " << res.compile_commands_path << "\n";
            }
            if (options.verbose && res.peak_memory_bytes > 0) {
                std::cout << "Peak Memory: " << exec::format_bytes(res.peak_memory_bytes) << "\n"
                          << "Duration:    " << res.duration_ms << " ms\n";
            }
        } else {
            std::cerr << "\n[x] Build failed (exit code " << res.exit_code << ").\n";
            for (const auto& diag : res.diagnostics) {
                std::cerr << "  - " << diag << "\n";
            }
            if (res.resource_limit_exceeded) {
                std::cerr << "\nAction suggestions:\n"
                          << "  - Reduce build parallelism with '--jobs 1'\n"
                          << "  - Review configured limit with '--max-memory <size>'\n"
                          << "  - Inspect WSL memory allocation in /etc/wsl.conf or .wslconfig if needed\n";
            }
        }
    }

    return res.success ? 0 : (res.exit_code != 0 ? res.exit_code : 1);
}

} // namespace nxdev::cli
