#include <nxdev/cli/cli.hpp>
#include <nxdev/cli/commands.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/config/host_config.hpp>
#include <nxdev/env/environment.hpp>
#include <iostream>
#include <iomanip>

namespace nxdev::cli {

CLI::CLI() {
    register_command(std::make_unique<VersionCommand>());
    register_command(std::make_unique<EnvCommand>());
    register_command(std::make_unique<DoctorCommand>());
    register_command(std::make_unique<ProjectCommand>());
    register_command(std::make_unique<ConfigCommand>());
    register_command(std::make_unique<ManifestCommand>());
    register_command(std::make_unique<ConfigureCommand>());
    register_command(std::make_unique<BuildCommand>());
    register_command(std::make_unique<CleanCommand>());
    register_command(std::make_unique<PackageCommand>());
    register_command(std::make_unique<PackCommand>());
    register_command(std::make_unique<NewCommand>());
    register_command(std::make_unique<DevicesCommand>());
    register_command(std::make_unique<RunCommand>());
    register_command(std::make_unique<DeployCommand>());
    register_command(std::make_unique<SymbolizeCommand>());
    register_command(std::make_unique<LogsCommand>());
    register_command(std::make_unique<InitCommand>());
    register_command(std::make_unique<PrepareCommand>());
    register_command(std::make_unique<SdkCommand>());
}

void CLI::register_command(std::unique_ptr<ICommand> cmd) {
    if (cmd) {
        std::string name(cmd->name());
        commands_[name] = std::move(cmd);
    }
}

void CLI::print_version() const {
#ifdef NXDEV_VERSION
    std::cout << "NXDev CLI version " << NXDEV_VERSION << "\n";
#else
    std::cout << "NXDev CLI version 0.1.0-dev\n";
#endif
    std::cout << "Unofficial Nintendo Switch Homebrew Development Ecosystem\n";
}

void CLI::print_help() const {
    std::cout << "NXDev - Unofficial Nintendo Switch Homebrew Development CLI\n\n";
    std::cout << "Usage: nxdev [global-options] <command> [command-options]\n\n";
    std::cout << "Available Commands:\n";
    for (const auto& [name, cmd] : commands_) {
        std::cout << "  " << std::left << std::setw(12) << name << " " << cmd->description() << "\n";
    }
    std::cout << "\nGlobal Options:\n";
    std::cout << "  --project <path>  Explicit path to project directory or nxapp.yaml\n";
    std::cout << "  --verbose         Enable detailed diagnostic logging\n";
    std::cout << "  --no-color        Disable colored output\n";
    std::cout << "  -h, --help        Display this help menu\n";
    std::cout << "  -v, --version     Display version information\n";
}

int CLI::run(int argc, char* argv[]) {
    if (argc <= 1) {
        print_help();
        return 0;
    }

    std::string command_name;
    std::vector<std::string> command_args;
    std::string explicit_project_path;
    bool verbose = false;
    bool no_color = false;

    // Parse global options and command name
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help" || arg == "help") {
            if (command_name.empty()) {
                print_help();
                return 0;
            } else {
                command_args.push_back(arg);
            }
        } else if (arg == "-v" || arg == "--version") {
            if (command_name.empty()) {
                print_version();
                return 0;
            } else {
                command_args.push_back(arg);
            }
        } else if (arg == "--project" || arg == "-p") {
            if (i + 1 < argc) {
                explicit_project_path = argv[++i];
            } else {
                std::cerr << "Error: '--project' option requires a path argument.\n";
                return 1;
            }
        } else if (arg == "--verbose" || arg == "-V") {
            verbose = true;
        } else if (arg == "--no-color") {
            no_color = true;
        } else if (command_name.empty()) {
            command_name = arg;
        } else {
            command_args.push_back(arg);
        }
    }

    if (command_name.empty()) {
        print_help();
        return 0;
    }

    // Resolve project (explicit or upward discovery)
    std::optional<project::NXDevProject> project;
    if (!explicit_project_path.empty()) {
        project = project::NXDevProject::load_explicit(explicit_project_path);
        if (!project.has_value()) {
            std::cerr << "Error: Specified project path does not exist or has no nxapp.yaml: "
                      << explicit_project_path << "\n";
            return 1;
        }
    } else {
        project = project::NXDevProject::discover();
    }

    // Load configuration with project root if available
    std::string proj_root = project.has_value() ? project->root_path() : "";
    auto host_cfg = config::HostConfig::load(proj_root);

    // Detect environment
    auto environment = env::Environment::detect(host_cfg);

    CommandContext ctx{
        .project = project,
        .env = environment,
        .config = host_cfg,
        .explicit_project_path = explicit_project_path,
        .verbose = verbose,
        .no_color = no_color
    };

    auto it = commands_.find(command_name);
    if (it != commands_.end()) {
        return it->second->execute(command_args, ctx);
    }

    std::cerr << "Error: Unknown command '" << command_name << "'. Run 'nxdev --help' for available commands.\n";
    return 1;
}

} // namespace nxdev::cli
