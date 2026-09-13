#include <nxdev/cli/commands.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/doctor/doctor.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;

namespace nxdev::cli {

namespace {

inline std::string c_green(std::string_view s) { return "\033[32m" + std::string(s) + "\033[0m"; }
inline std::string c_red(std::string_view s) { return "\033[31m" + std::string(s) + "\033[0m"; }
inline std::string c_yellow(std::string_view s) { return "\033[33m" + std::string(s) + "\033[0m"; }
inline std::string c_cyan(std::string_view s) { return "\033[36m" + std::string(s) + "\033[0m"; }
inline std::string c_bold(std::string_view s) { return "\033[1m" + std::string(s) + "\033[0m"; }

struct LinuxDistro {
    std::string id;
    std::string id_like;
    std::string name;
    std::string version;
};

LinuxDistro detect_linux_distro() {
    LinuxDistro d;
    d.id = "unknown";
    d.name = "Generic Linux";

    if (fs::exists("/etc/os-release")) {
        std::ifstream f("/etc/os-release");
        std::string line;
        while (std::getline(f, line)) {
            if (line.starts_with("ID=")) {
                d.id = line.substr(3);
                if (d.id.size() >= 2 && d.id.front() == '"' && d.id.back() == '"') {
                    d.id = d.id.substr(1, d.id.size() - 2);
                }
            } else if (line.starts_with("ID_LIKE=")) {
                d.id_like = line.substr(8);
                if (d.id_like.size() >= 2 && d.id_like.front() == '"' && d.id_like.back() == '"') {
                    d.id_like = d.id_like.substr(1, d.id_like.size() - 2);
                }
            } else if (line.starts_with("PRETTY_NAME=")) {
                d.name = line.substr(12);
                if (d.name.size() >= 2 && d.name.front() == '"' && d.name.back() == '"') {
                    d.name = d.name.substr(1, d.name.size() - 2);
                }
            } else if (line.starts_with("VERSION_ID=")) {
                d.version = line.substr(11);
                if (d.version.size() >= 2 && d.version.front() == '"' && d.version.back() == '"') {
                    d.version = d.version.substr(1, d.version.size() - 2);
                }
            }
        }
    }
    return d;
}

bool configure_devkitpro_environment(const std::string& dkp_root, const std::string& dka_root, std::string& configured_file) {
    std::string managed_block =
        "# >>> NXDev devkitPro >>>\n"
        "export DEVKITPRO=\"" + dkp_root + "\"\n"
        "export DEVKITA64=\"" + dka_root + "\"\n"
        "case \":$PATH:\" in\n"
        "    *\":$DEVKITA64/bin:\"*) ;;\n"
        "    *) export PATH=\"$DEVKITA64/bin:$PATH\" ;;\n"
        "esac\n"
        "case \":$PATH:\" in\n"
        "    *\":$DEVKITPRO/tools/bin:\"*) ;;\n"
        "    *) export PATH=\"$DEVKITPRO/tools/bin:$PATH\" ;;\n"
        "esac\n"
        "# <<< NXDev devkitPro <<<";

    auto update_file = [](const fs::path& target_path, const std::string& block) -> bool {
        try {
            if (!target_path.parent_path().empty() && !fs::exists(target_path.parent_path())) {
                fs::create_directories(target_path.parent_path());
            }
            std::string content;
            if (fs::exists(target_path)) {
                std::ifstream in(target_path);
                content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            }

            std::string tag_start = "# >>> NXDev devkitPro >>>";
            std::string tag_end = "# <<< NXDev devkitPro <<<";
            size_t start_pos = content.find(tag_start);
            size_t end_pos = content.find(tag_end);

            std::string new_content;
            if (start_pos != std::string::npos && end_pos != std::string::npos && end_pos >= start_pos) {
                new_content = content.substr(0, start_pos) + block + content.substr(end_pos + tag_end.length());
            } else {
                new_content = content;
                if (!new_content.empty() && new_content.back() != '\n') {
                    new_content += "\n";
                }
                new_content += "\n" + block + "\n";
            }

            std::ofstream out(target_path);
            if (!out) return false;
            out << new_content;
            return true;
        } catch (...) {
            return false;
        }
    };

    // 1. System-wide /etc/profile.d/devkitpro.sh if standard /opt/devkitpro and root / writable
    if (dkp_root == "/opt/devkitpro" && (geteuid() == 0 || (fs::exists("/etc/profile.d") && access("/etc/profile.d", W_OK) == 0))) {
        fs::path sys_profile = "/etc/profile.d/devkitpro.sh";
        if (update_file(sys_profile, managed_block)) {
            configured_file = sys_profile.string();
            return true;
        }
    }

    // 2. User shell profile
    const char* home = std::getenv("HOME");
    if (!home) return false;

    const char* shell_env = std::getenv("SHELL");
    std::string shell_str = shell_env ? shell_env : "/bin/bash";

    fs::path target_file;
    if (shell_str.find("zsh") != std::string::npos && fs::exists(fs::path(home) / ".zshrc")) {
        target_file = fs::path(home) / ".zshrc";
    } else if (shell_str.find("bash") != std::string::npos && fs::exists(fs::path(home) / ".bashrc")) {
        target_file = fs::path(home) / ".bashrc";
    } else if (fs::exists(fs::path(home) / ".bashrc")) {
        target_file = fs::path(home) / ".bashrc";
    } else {
        target_file = fs::path(home) / ".profile";
    }

    if (update_file(target_file, managed_block)) {
        configured_file = target_file.string();
        return true;
    }

    return false;
}

} // anonymous namespace

int PrepareCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    bool check_only = false;
    bool repair = false;
    bool yes_flag = false;
    bool dry_run = false;
    bool json_output = false;

    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: nxdev prepare [options]\n\n"
                      << "Install and configure devkitPro and Switch toolchain prerequisites on Linux host.\n\n"
                      << "Options:\n"
                      << "  --check       Verify toolchain readiness without making changes\n"
                      << "  --repair      Force reinstallation or resolution of inconsistent toolchain paths\n"
                      << "  -y, --yes     Automatic yes to prompts (for non-interactive / CI use)\n"
                      << "  --dry-run     Display commands that would be executed without running them\n"
                      << "  --json        Emit structured JSON output\n"
                      << "  -h, --help    Display this help message\n";
            return 0;
        } else if (arg == "--check") {
            check_only = true;
        } else if (arg == "--repair") {
            repair = true;
        } else if (arg == "-y" || arg == "--yes") {
            yes_flag = true;
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "--json") {
            json_output = true;
        }
    }

    // 1. Host Platform Verification
    bool is_linux_or_wsl = (ctx.env.host().os == env::OperatingSystem::Linux) || ctx.env.host().is_wsl;
    if (!is_linux_or_wsl) {
        if (json_output) {
            std::cout << "{\n"
                      << "  \"status\": \"error\",\n"
                      << "  \"message\": \"nxdev prepare is currently supported only on Linux environments.\",\n"
                      << "  \"platform\": \"" << ctx.env.host().os_name() << "\"\n"
                      << "}\n";
        } else {
            std::cerr << c_red("Error: ") << "nxdev prepare is currently supported only on Linux environments.\n"
                      << "For Windows hosts, please use Windows Subsystem for Linux (WSL2).\n"
                      << "For macOS hosts, please refer to https://devkitpro.org/wiki/Getting_Started.\n";
        }
        return 1;
    }

    // 2. Resolve Target Locations & Conflict Detection
    const char* cur_dkp_env = std::getenv("DEVKITPRO");
    const char* cur_dka_env = std::getenv("DEVKITA64");

    std::string target_dkp = "/opt/devkitpro";
    if (ctx.env.devkitpro().is_valid) {
        target_dkp = ctx.env.devkitpro().path;
    } else if (cur_dkp_env && *cur_dkp_env && fs::exists(cur_dkp_env)) {
        target_dkp = cur_dkp_env;
    }

    std::string target_dka = target_dkp + "/devkitA64";

    // Conflict detection
    if (cur_dkp_env && cur_dka_env && *cur_dkp_env && *cur_dka_env) {
        fs::path p_dkp(cur_dkp_env);
        fs::path p_dka(cur_dka_env);
        fs::path expected_dka = p_dkp / "devkitA64";
        bool matches = (p_dka.lexically_normal() == expected_dka.lexically_normal());
        std::error_code ec;
        if (!matches && fs::exists(p_dka, ec) && fs::exists(expected_dka, ec)) {
            matches = fs::equivalent(p_dka, expected_dka, ec);
        }

        if (!matches) {
            if (!repair && !yes_flag) {
                if (json_output) {
                    std::cout << "{\n"
                              << "  \"status\": \"conflict_detected\",\n"
                              << "  \"DEVKITPRO\": \"" << cur_dkp_env << "\",\n"
                              << "  \"DEVKITA64\": \"" << cur_dka_env << "\",\n"
                              << "  \"message\": \"Inconsistent DEVKITPRO and DEVKITA64 environment variables.\"\n"
                              << "}\n";
                } else {
                    std::cerr << c_red("Error: ") << "Conflict detected between environment variables:\n"
                              << "  DEVKITPRO=" << cur_dkp_env << "\n"
                              << "  DEVKITA64=" << cur_dka_env << "\n"
                              << "DEVKITA64 must belong to the selected DEVKITPRO installation (" << expected_dka.string() << ").\n"
                              << "Run 'nxdev prepare --repair' to resolve this conflict automatically.\n";
                }
                return 1;
            } else {
                target_dka = expected_dka.string();
            }
        }
    }

    // 3. Check existing toolchain health (Idempotency)
    bool has_devkitpro = ctx.env.devkitpro().is_valid;
    bool has_devkita64 = ctx.env.devkita64().is_valid && ctx.env.devkita64().is_consistent;
    bool has_libnx = ctx.env.libnx().found;
    bool has_switch_tools = ctx.env.switch_tools().is_found;

    const auto* gcc = ctx.env.get_tool("aarch64-none-elf-gcc");
    const auto* gxx = ctx.env.get_tool("aarch64-none-elf-g++");
    const auto* elf2nro = ctx.env.get_tool("elf2nro");
    const auto* nacptool = ctx.env.get_tool("nacptool");
    const auto* nxlink = ctx.env.get_tool("nxlink");

    bool fully_ready = has_devkitpro && has_devkita64 && has_libnx && has_switch_tools &&
                       (gcc && gcc->usable) && (gxx && gxx->usable) &&
                       (elf2nro && elf2nro->usable) && (nacptool && nacptool->usable) &&
                       (nxlink && nxlink->usable);

    if (fully_ready && !repair) {
        std::string configured_file;
        configure_devkitpro_environment(target_dkp, target_dka, configured_file);

        if (json_output) {
            std::cout << "{\n"
                      << "  \"status\": \"already_prepared\",\n"
                      << "  \"devkitPro\": \"" << ctx.env.devkitpro().path << "\",\n"
                      << "  \"devkitA64\": \"" << ctx.env.devkita64().path << "\",\n"
                      << "  \"configuredFile\": \"" << configured_file << "\",\n"
                      << "  \"message\": \"devkitPro toolchain is already installed and verified.\"\n"
                      << "}\n";
        } else {
            std::cout << c_green("✓") << " devkitPro toolchain is already installed and verified at "
                      << c_bold(ctx.env.devkitpro().path) << ".\n"
                      << "All required Switch compiler and packaging tools are operational.\n"
                      << "Run 'nxdev doctor' for detailed status or 'nxdev prepare --repair' to reinstall.\n";
        }
        return 0;
    }

    if (check_only) {
        if (json_output) {
            std::cout << "{\n"
                      << "  \"status\": \"missing_prerequisites\",\n"
                      << "  \"devkitProFound\": " << (has_devkitpro ? "true" : "false") << ",\n"
                      << "  \"devkitA64Found\": " << (has_devkita64 ? "true" : "false") << ",\n"
                      << "  \"libnxFound\": " << (has_libnx ? "true" : "false") << ",\n"
                      << "  \"switchToolsFound\": " << (has_switch_tools ? "true" : "false") << "\n"
                      << "}\n";
        } else {
            std::cout << c_yellow("!") << " Host environment requires preparation for Nintendo Switch development.\n"
                      << "Run 'nxdev prepare' to install devkitPro toolchain and dependencies.\n";
        }
        return 1;
    }

    // 4. Detect Distribution & Build Installation Commands
    LinuxDistro distro = detect_linux_distro();
    std::vector<std::string> install_steps;
    std::string package_manager = "apt";

    if (distro.id == "ubuntu" || distro.id == "debian" || distro.id_like.find("debian") != std::string::npos || distro.id_like.find("ubuntu") != std::string::npos) {
        package_manager = "apt";
        install_steps.push_back("sudo apt-get update");
        install_steps.push_back("sudo apt-get install -y curl gnupg ca-certificates xz-utils bzip2");
        install_steps.push_back("curl -fsSL https://apt.devkitpro.org/devkitpro-pub.gpg | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/devkitpro.gpg");
        install_steps.push_back("echo 'deb [signed-by=/etc/apt/trusted.gpg.d/devkitpro.gpg] https://apt.devkitpro.org stable main' | sudo tee /etc/apt/sources.list.d/devkitpro.list");
        install_steps.push_back("sudo apt-get update");
        install_steps.push_back("sudo apt-get install -y devkitpro-pacman");
        install_steps.push_back("sudo dkp-pacman -S --noconfirm switch-dev");
    } else if (distro.id == "arch" || distro.id_like.find("arch") != std::string::npos) {
        package_manager = "pacman";
        install_steps.push_back("sudo pacman-key --recv BC7D1E30425230E4");
        install_steps.push_back("sudo pacman-key --lsign-key BC7D1E30425230E4");
        install_steps.push_back("sudo pacman -S --noconfirm devkitA64 libnx switch-tools switch-pkg-config");
    } else if (distro.id == "fedora" || distro.id == "rhel" || distro.id_like.find("fedora") != std::string::npos || distro.id_like.find("rhel") != std::string::npos) {
        package_manager = "dnf";
        install_steps.push_back("sudo dnf install -y curl gnupg ca-certificates xz bzip2");
        install_steps.push_back("sudo dnf install -y https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.rpm");
        install_steps.push_back("sudo dkp-pacman -S --noconfirm switch-dev");
    } else {
        package_manager = "generic";
        install_steps.push_back("Refer to official guide: https://devkitpro.org/wiki/Getting_Started");
    }

    // 5. Interactive Confirmation
    if (!json_output) {
        std::cout << c_bold("=== NXDev Host Preparation ===") << "\n\n"
                  << "This will install and configure the devkitPro toolchain required for\n"
                  << "Nintendo Switch homebrew cross-compilation and packaging.\n\n"
                  << "Target Host:       " << distro.name << " (" << ctx.env.host().arch_name() << (ctx.env.host().is_wsl ? " / WSL2" : "") << ")\n"
                  << "Target Directory:  " << target_dkp << "\n"
                  << "Packages:          devkitA64, libnx, switch-tools, switch-pkg-config\n\n";

        if (dry_run) {
            std::cout << c_cyan("Planned execution commands (dry-run):") << "\n";
            for (const auto& cmd : install_steps) {
                std::cout << "  $ " << cmd << "\n";
            }
            return 0;
        }

        if (!yes_flag) {
            std::cout << "Do you want to proceed? [y/N] ";
            std::string ans;
            if (!std::getline(std::cin, ans) || (ans != "y" && ans != "Y" && ans != "yes" && ans != "Yes")) {
                std::cout << c_yellow("Preparation aborted by user.") << "\n";
                return 1;
            }
        }
    }

    // 6. Execution (with Mock Support for CI / Unit Tests)
    bool is_mock = (std::getenv("NXDEV_PREPARE_MOCK") != nullptr);
    if (!is_mock) {
        for (const auto& cmd : install_steps) {
            if (!json_output) {
                std::cout << c_cyan("==> ") << cmd << "\n";
            }
            int res = std::system(cmd.c_str());
            if (res != 0) {
                if (json_output) {
                    std::cout << "{\n"
                              << "  \"status\": \"error\",\n"
                              << "  \"failedCommand\": \"" << cmd << "\",\n"
                              << "  \"exitCode\": " << res << "\n"
                              << "}\n";
                } else {
                    std::cerr << c_red("Error: ") << "Command failed with exit code " << res << ": " << cmd << "\n";
                }
                return 1;
            }
        }
    }

    // 7. Environment Configuration
    std::string configured_file;
    configure_devkitpro_environment(target_dkp, target_dka, configured_file);

    // 8. Update Current Process Environment Immediately
    setenv("DEVKITPRO", target_dkp.c_str(), 1);
    setenv("DEVKITA64", target_dka.c_str(), 1);
    std::string curr_path = std::getenv("PATH") ? std::getenv("PATH") : "";
    std::string dka_bin = target_dka + "/bin";
    std::string tools_bin = target_dkp + "/tools/bin";
    if (curr_path.find(dka_bin) == std::string::npos) {
        curr_path = dka_bin + ":" + curr_path;
    }
    if (curr_path.find(tools_bin) == std::string::npos) {
        curr_path = tools_bin + ":" + curr_path;
    }
    setenv("PATH", curr_path.c_str(), 1);

    // 9. Post-Install Verification via Doctor
    auto new_env = env::Environment::detect(ctx.config);
    doctor::Doctor doc;
    doc.run_diagnostics(new_env, std::nullopt, doctor::DoctorProfile::Build);

    const auto* gcc_tool = new_env.get_tool("aarch64-none-elf-gcc");

    if (json_output) {
        std::cout << "{\n"
                  << "  \"status\": \"" << (new_env.devkitpro().is_valid && new_env.devkita64().is_valid ? "success" : "partial_success") << "\",\n"
                  << "  \"devkitPro\": \"" << target_dkp << "\",\n"
                  << "  \"devkitA64\": \"" << target_dka << "\",\n"
                  << "  \"configuredFile\": \"" << configured_file << "\",\n"
                  << "  \"doctorErrors\": " << doc.error_count() << "\n"
                  << "}\n";
    } else {
        std::cout << "\n" << c_bold("devkitPro Environment") << "\n"
                  << (new_env.devkitpro().is_valid ? "  " + c_green("[PASS] ") : "  " + c_red("[FAIL] ")) << "DEVKITPRO=" << target_dkp << "\n"
                  << (new_env.devkita64().is_valid && new_env.devkita64().is_consistent ? "  " + c_green("[PASS] ") : "  " + c_red("[FAIL] ")) << "DEVKITA64=" << target_dka << "\n"
                  << (gcc_tool && gcc_tool->usable ? "  " + c_green("[PASS] ") : "  " + c_yellow("[INFO] ")) << "aarch64-none-elf-gcc (" << (gcc_tool && gcc_tool->usable ? gcc_tool->version : "not in PATH") << ")\n"
                  << (new_env.libnx().found ? "  " + c_green("[PASS] ") : "  " + c_yellow("[INFO] ")) << "libnx (" << new_env.libnx().version << ")\n"
                  << (new_env.switch_tools().is_found ? "  " + c_green("[PASS] ") : "  " + c_yellow("[INFO] ")) << "switch-tools\n\n";

        if (!configured_file.empty()) {
            std::cout << "✓ Persistent environment configured in " << c_cyan(configured_file) << "\n";
        }
        std::cout << "You can now create and build Switch homebrew projects:\n"
                  << "  $ " << c_cyan("nxdev new my-app") << "\n"
                  << "  $ " << c_cyan("cd my-app && nxdev build") << "\n";
    }

    return (doc.error_count() == 0 || is_mock) ? 0 : 1;
}

} // namespace nxdev::cli
