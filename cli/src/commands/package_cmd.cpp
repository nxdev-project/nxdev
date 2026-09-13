#include <nxdev/cli/commands.hpp>
#include <nxdev/packages/registry.hpp>
#include <nxdev/packages/package_manager.hpp>
#include <nxdev/packages/resolver.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace nxdev::cli {

static std::string escape_json(std::string_view s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

int PackageCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (args.empty() || args[0] == "-h" || args[0] == "--help" || args[0] == "help") {
        std::cout << "NXDev Package Manager - Manage SDK modules and devkitPro portlib dependencies\n\n"
                  << "Usage: nxdev package <subcommand> [options]\n\n"
                  << "Subcommands:\n"
                  << "  list [filter] [--json]         List supported NXDev logical modules and their status\n"
                  << "  info <id> [--json]             Show detailed module definition, dependencies, and targets\n"
                  << "  status [--json]                Check installation status of current project dependencies\n"
                  << "  search <query> [--json]        Search for supported modules by name, keyword, or portlib\n"
                  << "  install <id...> [--dry-run]    Install specified module(s) via devkitPro package manager\n"
                  << "  install --missing [--dry-run]  Install all missing dependencies declared in nxapp.yaml\n"
                  << "  remove <id...> [--dry-run]     Remove specified devkitPro portlib module(s)\n\n"
                  << "Options:\n"
                  << "  --json                         Output result in structured JSON format\n"
                  << "  --dry-run                      Show planned actions without modifying the host system\n"
                  << "  -y, --yes                      Non-interactive mode (automatically confirm actions)\n";
        return 0;
    }

    std::string subcmd = args[0];
    bool json_output = false;
    bool dry_run = false;
    bool non_interactive = false;
    bool missing_flag = false;
    std::vector<std::string> positional;

    for (size_t i = 1; i < args.size(); ++i) {
        const auto& a = args[i];
        if (a == "--json") {
            json_output = true;
        } else if (a == "--dry-run") {
            dry_run = true;
        } else if (a == "-y" || a == "--yes") {
            non_interactive = true;
        } else if (a == "--missing") {
            missing_flag = true;
        } else if (a == "-h" || a == "--help") {
            // subcmd help
            std::cout << "Usage: nxdev package " << subcmd << " [options]\n";
            return 0;
        } else {
            positional.push_back(a);
        }
    }

    // Load registry
    auto reg_res = packages::PackageRegistry::load_default(ctx.env);
    if (!reg_res.is_success()) {
        if (json_output) {
            std::cout << "{\n  \"error\": \"" << escape_json(reg_res.error_name()) << "\"\n}\n";
        } else {
            std::cerr << "Error loading package registry: " << reg_res.error_name() << "\n";
        }
        return 1;
    }

    const auto& registry = reg_res.value();
    packages::PackageManager pm(ctx.env, registry);

    // -------------------------------------------------------------------------
    // Subcommand: list
    // -------------------------------------------------------------------------
    if (subcmd == "list") {
        std::string filter = positional.empty() ? "" : positional[0];
        auto matches = registry.search(filter);

        if (json_output) {
            std::cout << "[\n";
            for (size_t i = 0; i < matches.size(); ++i) {
                const auto* pkg = matches[i];
                auto status_info = pm.check_module_status(*pkg);
                std::cout << status_info.to_json() << (i + 1 < matches.size() ? ",\n" : "\n");
            }
            std::cout << "]\n";
            return 0;
        }

        std::cout << "NXDev Supported Modules (" << matches.size() << " available)\n\n";
        std::cout << std::left
                  << std::setw(20) << "Module ID"
                  << std::setw(12) << "Type"
                  << std::setw(14) << "Category"
                  << std::setw(16) << "Status"
                  << "Description\n";
        std::cout << std::string(80, '-') << "\n";

        for (const auto* pkg : matches) {
            auto status_info = pm.check_module_status(*pkg);
            std::string status_str = packages::package_status_to_string(status_info.status);
            if (!status_info.detected_version.empty() && status_info.detected_version != "built-in") {
                status_str += " (" + status_info.detected_version + ")";
            }

            std::cout << std::left
                      << std::setw(20) << pkg->id
                      << std::setw(12) << packages::package_kind_to_string(pkg->kind)
                      << std::setw(14) << pkg->category
                      << std::setw(16) << status_str
                      << pkg->description << "\n";
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // Subcommand: info
    // -------------------------------------------------------------------------
    if (subcmd == "info") {
        if (positional.empty()) {
            if (json_output) {
                std::cout << "{\n  \"error\": \"Missing module ID argument\"\n}\n";
            } else {
                std::cerr << "Error: 'nxdev package info' requires a module ID argument (e.g. nxdev.sdl2).\n";
            }
            return 1;
        }

        std::string target_id = positional[0];
        const auto* pkg = registry.find(target_id);
        if (!pkg) {
            if (json_output) {
                std::cout << "{\n  \"error\": \"Unknown module: " << escape_json(target_id) << "\"\n}\n";
            } else {
                std::cerr << "Error: Unknown module '" << target_id << "'. Run 'nxdev package list' for available modules.\n";
            }
            return 1;
        }

        auto status_info = pm.check_module_status(*pkg);

        if (json_output) {
            std::cout << status_info.to_json() << "\n";
            return 0;
        }

        std::cout << "NXDev Module: " << pkg->id << " (" << pkg->name << ")\n";
        std::cout << "--------------------------------------------------------\n";
        std::cout << "Description:       " << pkg->description << "\n";
        std::cout << "Kind:              " << packages::package_kind_to_string(pkg->kind) << "\n";
        std::cout << "Category:          " << pkg->category << "\n";
        std::cout << "Status:            " << packages::package_status_to_string(status_info.status);
        if (!status_info.detected_version.empty()) {
            std::cout << " (" << status_info.detected_version << ")";
        }
        std::cout << "\n";

        std::cout << "Dependencies:      ";
        if (pkg->dependencies.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < pkg->dependencies.size(); ++i) {
                std::cout << pkg->dependencies[i] << (i + 1 < pkg->dependencies.size() ? ", " : "\n");
            }
        }

        if (pkg->is_devkitpro()) {
            std::cout << "devkitPro Packages:";
            for (const auto& dkp : pkg->devkitpro.packages) {
                std::cout << " " << dkp;
            }
            std::cout << "\n";
        }

        std::cout << "CMake Targets:     ";
        if (pkg->cmake.targets.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < pkg->cmake.targets.size(); ++i) {
                std::cout << pkg->cmake.targets[i] << (i + 1 < pkg->cmake.targets.size() ? ", " : "\n");
            }
        }

        if (!pkg->license.empty()) {
            std::cout << "License:           " << pkg->license << "\n";
        }
        if (!pkg->upstream_url.empty()) {
            std::cout << "Upstream URL:      " << pkg->upstream_url << "\n";
        }

        if (!status_info.missing_artifacts.empty()) {
            std::cout << "\nMissing Artifacts:\n";
            for (const auto& a : status_info.missing_artifacts) {
                std::cout << "  - " << a << "\n";
            }
        }

        return 0;
    }

    // -------------------------------------------------------------------------
    // Subcommand: search
    // -------------------------------------------------------------------------
    if (subcmd == "search") {
        std::string query = positional.empty() ? "" : positional[0];
        auto matches = registry.search(query);

        if (json_output) {
            std::cout << "[\n";
            for (size_t i = 0; i < matches.size(); ++i) {
                std::cout << matches[i]->to_json() << (i + 1 < matches.size() ? ",\n" : "\n");
            }
            std::cout << "]\n";
            return 0;
        }

        std::cout << "Found " << matches.size() << " matching module(s) for '" << query << "':\n\n";
        for (const auto* pkg : matches) {
            std::cout << "  * " << std::left << std::setw(20) << pkg->id
                      << " [" << packages::package_kind_to_string(pkg->kind) << "] "
                      << pkg->name << " - " << pkg->description << "\n";
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // Subcommand: status
    // -------------------------------------------------------------------------
    if (subcmd == "status") {
        if (!ctx.project.has_value() || !ctx.project->is_valid()) {
            // Check all modules
            auto all_statuses = pm.check_all_statuses();
            if (json_output) {
                std::cout << "[\n";
                for (size_t i = 0; i < all_statuses.size(); ++i) {
                    std::cout << all_statuses[i].to_json() << (i + 1 < all_statuses.size() ? ",\n" : "\n");
                }
                std::cout << "]\n";
                return 0;
            }

            std::cout << "NXDev Environment Module Status (No active project discovered):\n\n";
            std::cout << std::left
                      << std::setw(20) << "Module ID"
                      << std::setw(12) << "Type"
                      << std::setw(16) << "Status"
                      << "Details\n";
            std::cout << std::string(75, '-') << "\n";

            for (const auto& s : all_statuses) {
                std::string st = packages::package_status_to_string(s.status);
                std::string details = s.detected_version.empty() ? "" : "version: " + s.detected_version;
                if (s.status == packages::PackageStatus::Missing && !s.missing_system_packages.empty()) {
                    details = "requires: " + s.missing_system_packages[0];
                }
                std::cout << std::left
                          << std::setw(20) << s.definition.id
                          << std::setw(12) << packages::package_kind_to_string(s.definition.kind)
                          << std::setw(16) << st
                          << details << "\n";
            }
            return 0;
        }

        // Project dependencies status
        const auto& manifest = ctx.project->manifest();
        auto statuses = pm.check_manifest_dependencies(manifest);

        if (json_output) {
            std::cout << "[\n";
            for (size_t i = 0; i < statuses.size(); ++i) {
                std::cout << statuses[i].to_json() << (i + 1 < statuses.size() ? ",\n" : "\n");
            }
            std::cout << "]\n";
            return 0;
        }

        std::cout << "Project Dependencies Status: " << manifest.application().name << "\n";
        std::cout << "Manifest: " << ctx.project->manifest_path() << "\n\n";
        std::cout << std::left
                  << std::setw(20) << "Module ID"
                  << std::setw(12) << "Type"
                  << std::setw(16) << "Status"
                  << "Details\n";
        std::cout << std::string(75, '-') << "\n";

        bool any_missing = false;
        for (const auto& s : statuses) {
            std::string st = packages::package_status_to_string(s.status);
            std::string details = s.detected_version.empty() ? "" : "version: " + s.detected_version;
            if (s.status == packages::PackageStatus::Missing) {
                any_missing = true;
                if (!s.missing_system_packages.empty()) {
                    details = "run 'nxdev package install " + s.definition.id + "'";
                }
            } else if (s.status == packages::PackageStatus::Incomplete) {
                any_missing = true;
                details = "incomplete installation (missing headers/libs)";
            }

            std::cout << std::left
                      << std::setw(20) << s.definition.id
                      << std::setw(12) << packages::package_kind_to_string(s.definition.kind)
                      << std::setw(16) << st
                      << details << "\n";
        }

        if (any_missing) {
            std::cout << "\nTip: Run 'nxdev package install --missing' to install missing project dependencies.\n";
        } else {
            std::cout << "\nAll project dependencies are ready!\n";
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // Subcommand: install
    // -------------------------------------------------------------------------
    if (subcmd == "install") {
        packages::PackageOperationResult result;

        if (missing_flag) {
            if (!ctx.project.has_value() || !ctx.project->is_valid()) {
                if (json_output) {
                    std::cout << "{\n  \"error\": \"No valid NXDev project found in current directory\"\n}\n";
                } else {
                    std::cerr << "Error: 'nxdev package install --missing' requires an active project containing nxapp.yaml.\n";
                }
                return 1;
            }
            result = pm.install_missing(ctx.project->manifest(), dry_run, non_interactive);
        } else {
            if (positional.empty()) {
                if (json_output) {
                    std::cout << "{\n  \"error\": \"No modules specified to install\"\n}\n";
                } else {
                    std::cerr << "Error: Specify module(s) to install (e.g. 'nxdev package install nxdev.sdl2') or use '--missing'.\n";
                }
                return 1;
            }
            result = pm.install(positional, dry_run, non_interactive);
        }

        if (json_output) {
            std::cout << result.to_json() << "\n";
            return result.success ? 0 : 1;
        }

        if (result.dry_run) {
            std::cout << "==> Dry Run: Package Installation Plan\n";
            std::cout << "  Resolved Modules: ";
            for (const auto& m : result.resolved_modules) std::cout << m << " ";
            std::cout << "\n  devkitPro Packages to Install: ";
            if (result.system_packages_to_install.empty()) {
                std::cout << "(none - already up to date)\n";
            } else {
                for (const auto& p : result.system_packages_to_install) std::cout << p << " ";
                std::cout << "\n";
            }
            if (!result.command_executed.empty()) {
                std::cout << "  Command to run: ";
                for (const auto& c : result.command_executed) std::cout << c << " ";
                std::cout << "\n";
            }
            return 0;
        }

        for (const auto& msg : result.messages) {
            std::cout << msg << "\n";
        }

        return result.success ? 0 : 1;
    }

    // -------------------------------------------------------------------------
    // Subcommand: remove
    // -------------------------------------------------------------------------
    if (subcmd == "remove") {
        if (positional.empty()) {
            if (json_output) {
                std::cout << "{\n  \"error\": \"No modules specified to remove\"\n}\n";
            } else {
                std::cerr << "Error: Specify module(s) to remove (e.g. 'nxdev package remove nxdev.sdl2').\n";
            }
            return 1;
        }

        auto result = pm.remove(positional, dry_run, non_interactive);

        if (json_output) {
            std::cout << result.to_json() << "\n";
            return result.success ? 0 : 1;
        }

        if (result.dry_run) {
            std::cout << "==> Dry Run: Package Removal Plan\n";
            std::cout << "  devkitPro Packages to Remove: ";
            for (const auto& p : result.system_packages_to_remove) std::cout << p << " ";
            std::cout << "\n";
            if (!result.command_executed.empty()) {
                std::cout << "  Command to run: ";
                for (const auto& c : result.command_executed) std::cout << c << " ";
                std::cout << "\n";
            }
            return 0;
        }

        for (const auto& msg : result.messages) {
            std::cout << msg << "\n";
        }

        return result.success ? 0 : 1;
    }

    if (json_output) {
        std::cout << "{\n  \"error\": \"Unknown package subcommand: " << escape_json(subcmd) << "\"\n}\n";
    } else {
        std::cerr << "Error: Unknown package subcommand '" << subcmd << "'. Run 'nxdev package --help' for usage.\n";
    }
    return 1;
}

} // namespace nxdev::cli
