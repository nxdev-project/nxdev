#include <nxdev/cli/cli.hpp>
#include <nxdev/cli/commands.hpp>
#include <iostream>
#include <iomanip>

namespace nxdev::cli {

CLI::CLI() {
    register_command(std::make_unique<VersionCommand>());
    register_command(std::make_unique<DoctorCommand>());
    register_command(std::make_unique<BuildCommand>());
    register_command(std::make_unique<PackCommand>());
    register_command(std::make_unique<NewCommand>());
    register_command(std::make_unique<ManifestCommand>());
}

void CLI::register_command(std::unique_ptr<ICommand> cmd) {
    if (cmd) {
        std::string name(cmd->name());
        commands_[name] = std::move(cmd);
    }
}

void CLI::print_version() const {
    auto it = commands_.find("version");
    if (it != commands_.end()) {
        std::vector<std::string> dummy_args;
        it->second->execute(dummy_args);
    } else {
        std::cout << "NXDev version 0.1.0-dev\n";
    }
}

void CLI::print_help() const {
    std::cout << "NXDev - Unofficial Nintendo Switch Homebrew Development CLI\n\n";
    std::cout << "Usage: nxdev [command] [options]\n\n";
    std::cout << "Available Commands:\n";
    for (const auto& [name, cmd] : commands_) {
        std::cout << "  " << std::left << std::setw(12) << name << " " << cmd->description() << "\n";
    }
    std::cout << "\nGlobal Flags:\n";
    std::cout << "  -h, --help      Display this help menu\n";
    std::cout << "  -v, --version   Display version information\n";
}

int CLI::run(int argc, char* argv[]) {
    if (argc <= 1) {
        print_help();
        return 0;
    }

    std::string first_arg = argv[1];

    if (first_arg == "-h" || first_arg == "--help" || first_arg == "help") {
        print_help();
        return 0;
    }

    if (first_arg == "-v" || first_arg == "--version") {
        print_version();
        return 0;
    }

    auto it = commands_.find(first_arg);
    if (it != commands_.end()) {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return it->second->execute(args);
    }

    std::cerr << "Error: Unknown command '" << first_arg << "'. Run 'nxdev --help' for available commands.\n";
    return 1;
}

} // namespace nxdev::cli
