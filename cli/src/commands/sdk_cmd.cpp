#include <nxdev/cli/commands.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/exec/resource_policy.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <nxdev/exec/controlled_runner.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <thread>

namespace fs = std::filesystem;

namespace nxdev::cli {

namespace {

inline std::string c_green(std::string_view s) { return "\033[32m" + std::string(s) + "\033[0m"; }
inline std::string c_red(std::string_view s) { return "\033[31m" + std::string(s) + "\033[0m"; }
inline std::string c_cyan(std::string_view s) { return "\033[36m" + std::string(s) + "\033[0m"; }
inline std::string c_bold(std::string_view s) { return "\033[1m" + std::string(s) + "\033[0m"; }

const char* INSTALL_SH_CONTENT = R"SH(#!/usr/bin/env bash
set -euo pipefail

# NXDevSDK Installer
# Unofficial Nintendo Switch Homebrew Development SDK

DEFAULT_PREFIX="/opt/nxdev"
PREFIX="${DEFAULT_PREFIX}"
FORCE=0
NON_INTERACTIVE=0
CHECK_ONLY=0
NO_ENV=0

print_help() {
    echo "NXDevSDK Linux / WSL Installer"
    echo "Usage: ./install.sh [options]"
    echo ""
    echo "Options:"
    echo "  --prefix <path>   Custom installation target directory (default: ${DEFAULT_PREFIX})"
    echo "  --no-env          Skip persistent environment and shell profile configuration"
    echo "  -f, --force       Overwrite existing SDK without prompting"
    echo "  -y, --yes         Non-interactive mode"
    echo "  --check           Validate SDK.zip payload without installing"
    echo "  -h, --help        Display this help message"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix)
            PREFIX="$2"
            shift 2
            ;;
        --no-env)
            NO_ENV=1
            shift
            ;;
        -f|--force)
            FORCE=1
            shift
            ;;
        -y|--yes)
            NON_INTERACTIVE=1
            shift
            ;;
        --check)
            CHECK_ONLY=1
            shift
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ZIP="${SCRIPT_DIR}/SDK.zip"

if [[ ! -f "${SDK_ZIP}" ]]; then
    echo "Error: SDK.zip not found in ${SCRIPT_DIR}" >&2
    exit 1
fi

if ! command -v unzip >/dev/null 2>&1; then
    echo "Error: 'unzip' utility is required for extraction." >&2
    exit 1
fi

TEMP_STAGE="$(mktemp -d /tmp/nxdev-sdk-stage.XXXXXX)"
trap 'rm -rf "${TEMP_STAGE}"' EXIT

unzip -q "${SDK_ZIP}" -d "${TEMP_STAGE}"

if [[ ! -f "${TEMP_STAGE}/share/nxdev/sdk.json" ]]; then
    echo "Error: Invalid SDK payload (missing share/nxdev/sdk.json)" >&2
    exit 1
fi

SDK_VERSION="$(grep '"version"' "${TEMP_STAGE}/share/nxdev/sdk.json" | head -n1 | cut -d '"' -f 4 || echo "0.1.0-beta.1")"

if [[ ${CHECK_ONLY} -eq 1 ]]; then
    echo "SDK.zip is valid. Packaged version: ${SDK_VERSION}"
    exit 0
fi

echo "=== Installing NXDevSDK ${SDK_VERSION} ==="
echo "Target location: ${PREFIX}"

if [[ -d "${PREFIX}" && ${FORCE} -eq 0 && ${NON_INTERACTIVE} -eq 0 ]]; then
    if [[ -f "${PREFIX}/share/nxdev/sdk.json" ]]; then
        EXISTING_VER="$(grep '"version"' "${PREFIX}/share/nxdev/sdk.json" | head -n1 | cut -d '"' -f 4 || echo "unknown")"
        echo "Detected existing NXDevSDK (${EXISTING_VER}) at ${PREFIX}."
    fi
    read -p "Overwrite existing installation? [y/N] " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 1
    fi
fi

SUDO=""
if [[ ! -w "$(dirname "${PREFIX}")" && ! -w "${PREFIX}" ]]; then
    if [[ $EUID -ne 0 ]]; then
        SUDO="sudo"
    fi
fi

${SUDO} mkdir -p "${PREFIX}"
${SUDO} rm -rf "${PREFIX:?}"/*
${SUDO} cp -R "${TEMP_STAGE}"/* "${PREFIX}/"
${SUDO} chmod -R u+rwX,go+rX "${PREFIX}"

echo "✓ NXDevSDK ${SDK_VERSION} installed successfully to ${PREFIX}!"

# Environment Configuration
update_managed_block() {
    local target_file="$1"
    local block_content="$2"
    local use_sudo="${3:-0}"

    local dir
    dir="$(dirname "${target_file}")"
    if [[ ! -d "${dir}" ]]; then
        if [[ ${use_sudo} -eq 1 ]]; then
            sudo mkdir -p "${dir}" 2>/dev/null || return 1
        else
            mkdir -p "${dir}" 2>/dev/null || return 1
        fi
    fi

    if [[ ! -f "${target_file}" ]]; then
        if [[ ${use_sudo} -eq 1 ]]; then
            sudo touch "${target_file}" 2>/dev/null || return 1
        else
            touch "${target_file}" 2>/dev/null || return 1
        fi
    fi

    local tmp_file
    tmp_file="$(mktemp /tmp/nxdev-env.XXXXXX)"

    if grep -q "# >>> NXDevSDK >>>" "${target_file}" 2>/dev/null; then
        awk '
            /# >>> NXDevSDK >>>/ { in_block=1; next }
            /# <<< NXDevSDK <<</ { in_block=0; next }
            !in_block { print }
        ' "${target_file}" > "${tmp_file}"
        echo "" >> "${tmp_file}"
        echo "${block_content}" >> "${tmp_file}"
    else
        cat "${target_file}" > "${tmp_file}"
        echo "" >> "${tmp_file}"
        echo "${block_content}" >> "${tmp_file}"
    fi

    if [[ ${use_sudo} -eq 1 ]]; then
        sudo cp "${tmp_file}" "${target_file}"
        sudo chmod 644 "${target_file}"
    else
        cp "${tmp_file}" "${target_file}"
        chmod 644 "${target_file}" 2>/dev/null || true
    fi
    rm -f "${tmp_file}"
    return 0
}

MANAGED_ENV_BLOCK="# >>> NXDevSDK >>>
export NXDEV_SDK_ROOT=\"${PREFIX}\"
case \":\$PATH:\" in
    *\":\$NXDEV_SDK_ROOT/bin:\"*) ;;
    *) export PATH=\"\$NXDEV_SDK_ROOT/bin:\$PATH\" ;;
esac
# <<< NXDevSDK <<<"

ENV_CONFIGURED=0
CONFIGURED_FILE=""

if [[ ${NO_ENV} -eq 0 ]]; then
    # System-level installation into /opt/nxdev or root-owned system prefix
    if [[ "${PREFIX}" == "${DEFAULT_PREFIX}" || ${EUID} -eq 0 ]] && [[ -d "/etc/profile.d" || ${EUID} -eq 0 || -n "${SUDO}" ]]; then
        USE_SUDO_FOR_PROFILE=0
        if [[ ${EUID} -ne 0 ]]; then
            USE_SUDO_FOR_PROFILE=1
        fi
        if update_managed_block "/etc/profile.d/nxdev.sh" "${MANAGED_ENV_BLOCK}" ${USE_SUDO_FOR_PROFILE}; then
            ENV_CONFIGURED=1
            CONFIGURED_FILE="/etc/profile.d/nxdev.sh"
        fi
    fi

    # User-level installation or user profile configuration
    if [[ ${ENV_CONFIGURED} -eq 0 ]]; then
        USER_HOME="${HOME:-}"
        if [[ -z "${USER_HOME}" && -n "${SUDO_USER:-}" ]]; then
            USER_HOME="$(getent passwd "${SUDO_USER}" | cut -d: -f6 2>/dev/null || true)"
        fi

        if [[ -n "${USER_HOME}" && -d "${USER_HOME}" ]]; then
            TARGET_SHELL="${SHELL:-/bin/bash}"
            TARGET_RC=""
            if [[ "${TARGET_SHELL}" == *"zsh"* && -f "${USER_HOME}/.zshrc" ]]; then
                TARGET_RC="${USER_HOME}/.zshrc"
            elif [[ "${TARGET_SHELL}" == *"bash"* && -f "${USER_HOME}/.bashrc" ]]; then
                TARGET_RC="${USER_HOME}/.bashrc"
            elif [[ -f "${USER_HOME}/.bashrc" ]]; then
                TARGET_RC="${USER_HOME}/.bashrc"
            elif [[ -f "${USER_HOME}/.profile" ]]; then
                TARGET_RC="${USER_HOME}/.profile"
            else
                TARGET_RC="${USER_HOME}/.bashrc"
            fi

            if update_managed_block "${TARGET_RC}" "${MANAGED_ENV_BLOCK}" 0; then
                ENV_CONFIGURED=1
                CONFIGURED_FILE="${TARGET_RC}"
            fi
        fi
    fi
fi

echo ""
if [[ ${ENV_CONFIGURED} -eq 1 ]]; then
    echo "✓ Environment successfully configured in: ${CONFIGURED_FILE}"
    echo "  - NXDEV_SDK_ROOT=\"${PREFIX}\""
    echo "  - PATH includes \"\$NXDEV_SDK_ROOT/bin\""
    echo ""
    echo "Note: A new shell session may be required for environment changes to take effect."
    echo "To activate immediately in your current terminal:"
    echo "  export NXDEV_SDK_ROOT=\"${PREFIX}\""
    echo "  export PATH=\"\$NXDEV_SDK_ROOT/bin:\$PATH\""
elif [[ ${NO_ENV} -eq 1 ]]; then
    echo "Skipped environment configuration (--no-env specified)."
    echo "To configure manually, add the following to your shell profile:"
    echo "  export NXDEV_SDK_ROOT=\"${PREFIX}\""
    echo "  export PATH=\"\$NXDEV_SDK_ROOT/bin:\$PATH\""
else
    echo "Note: Could not automatically modify shell configuration."
    echo "Please add the following lines to your ~/.bashrc or ~/.profile:"
    echo ""
    echo "  # >>> NXDevSDK >>>"
    echo "  export NXDEV_SDK_ROOT=\"${PREFIX}\""
    echo "  export PATH=\"\$NXDEV_SDK_ROOT/bin:\$PATH\""
    echo "  # <<< NXDevSDK <<<"
fi

echo ""
echo "To build Nintendo Switch projects using this SDK:"
echo "  find_package(NXDev CONFIG REQUIRED)"
echo "  nxdev_add_application(my-app SOURCES src/main.cpp)"
echo ""
echo "To verify toolchain status:"
echo "  nxdev doctor"
)SH";

} // anonymous namespace

int SdkCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        std::cout << "Usage: nxdev sdk <subcommand> [options]\n\n"
                  << "Inspect or package the NXDevSDK distribution archive, or build third-party backends.\n\n"
                  << "Subcommands:\n"
                  << "  info                 Display detected NXDevSDK location, version, and metadata\n"
                  << "  package              Build distributable NXDevSDK.zip archive\n"
                  << "  build-hacbrewpack     Build hacBrewPack NSP packaging backend with resource limits\n";
        return 0;
    }

    std::string subcmd = args[0];
    bool json_output = false;
    for (const auto& a : args) {
        if (a == "--json") json_output = true;
    }

    if (subcmd == "info") {
        if (json_output) {
            std::cout << "{\n"
                      << "  \"found\": " << (ctx.env.sdk().found ? "true" : "false") << ",\n"
                      << "  \"isInstalled\": " << (ctx.env.sdk().is_installed ? "true" : "false") << ",\n"
                      << "  \"isValid\": " << (ctx.env.sdk().is_valid ? "true" : "false") << ",\n"
                      << "  \"path\": \"" << ctx.env.sdk().path << "\",\n"
                      << "  \"version\": \"" << ctx.env.sdk().version << "\",\n"
                      << "  \"layoutVersion\": " << ctx.env.sdk().layout_version << ",\n"
                      << "  \"source\": \"" << ctx.env.sdk().source_name() << "\"\n"
                      << "}\n";
        } else {
            std::cout << c_bold("=== NXDevSDK Details ===") << "\n"
                      << "SDK Status:     " << (ctx.env.sdk().is_valid ? c_green("Valid") : c_red("Not Found / Invalid")) << "\n"
                      << "SDK Location:   " << (ctx.env.sdk().found ? ctx.env.sdk().path : "N/A") << "\n"
                      << "SDK Version:    " << ctx.env.sdk().version << "\n"
                      << "Layout Version: " << ctx.env.sdk().layout_version << "\n"
                      << "Source:         " << ctx.env.sdk().source_name() << "\n"
                      << "Mode:           " << (ctx.env.sdk().is_installed ? "Installed Distribution" : "Repository Source Tree") << "\n";
        }
        return ctx.env.sdk().is_valid ? 0 : 1;
    }

    if (subcmd == "package") {
        fs::path out_dir = fs::current_path() / "dist" / "sdk";
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--output" && i + 1 < args.size()) {
                out_dir = fs::absolute(args[++i]);
            }
        }

        try {
            fs::create_directories(out_dir);
        } catch (const std::exception& e) {
            std::cerr << c_red("Error: ") << "Failed to create output directory " << out_dir << ": " << e.what() << "\n";
            return 1;
        }

        // Locate repository root
        fs::path repo_root;
        fs::path p = fs::current_path();
        for (int i = 0; i < 4 && p != p.root_path(); ++i) {
            if (fs::exists(p / "sdk" / "core" / "include" / "nxdev" / "nxdev.hpp")) {
                repo_root = p;
                break;
            }
            p = p.parent_path();
        }

        if (repo_root.empty()) {
            std::cerr << c_red("Error: ") << "Cannot locate NXDev source tree to package SDK.\n";
            return 1;
        }

        fs::path stage_dir = out_dir / ".sdk_stage";
        fs::path inner_stage = stage_dir / "sdk_payload";
        fs::path outer_stage = stage_dir / "outer_payload";

        try {
            fs::remove_all(stage_dir);
            fs::create_directories(inner_stage / "bin");
            fs::create_directories(inner_stage / "include" / "nxdev");
            fs::create_directories(inner_stage / "src" / "core");
            fs::create_directories(inner_stage / "src" / "modules");
            fs::create_directories(inner_stage / "share" / "nxdev" / "cmake" / "toolchains");
            fs::create_directories(inner_stage / "share" / "nxdev" / "schemas");
            fs::create_directories(inner_stage / "share" / "nxdev" / "package-registry");
            fs::create_directories(inner_stage / "share" / "nxdev" / "templates");
            fs::create_directories(inner_stage / "licenses");
            fs::create_directories(outer_stage);

            // 0. Copy Host Binaries (nxdev and hacbrewpack)
            fs::path host_bin;
            if (fs::exists(repo_root / "build" / "bin" / "nxdev")) {
                host_bin = repo_root / "build" / "bin" / "nxdev";
            } else if (fs::exists(repo_root / "build" / "cli" / "nxdev")) {
                host_bin = repo_root / "build" / "cli" / "nxdev";
            } else if (fs::exists("/proc/self/exe")) {
                host_bin = "/proc/self/exe";
            }

            if (!host_bin.empty() && fs::exists(host_bin)) {
                fs::copy_file(host_bin, inner_stage / "bin" / "nxdev", fs::copy_options::overwrite_existing);
                fs::permissions(inner_stage / "bin" / "nxdev",
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec);
            }

            // Copy hacbrewpack binary
            fs::path hbp_bin;
            if (fs::exists(repo_root / "build" / "bin" / "hacbrewpack")) {
                hbp_bin = repo_root / "build" / "bin" / "hacbrewpack";
            } else if (fs::exists(repo_root / "build" / "third_party" / "hacbrewpack" / "hacbrewpack")) {
                hbp_bin = repo_root / "build" / "third_party" / "hacbrewpack" / "hacbrewpack";
            } else if (const auto* t = ctx.env.get_tool("hacbrewpack"); t && t->usable) {
                hbp_bin = t->path;
            }

            if (hbp_bin.empty() || !fs::exists(hbp_bin)) {
                // If hacbrewpack is not yet built, build it now
                std::vector<std::string> build_args = {"build-hacbrewpack"};
                if (ctx.verbose) build_args.push_back("--verbose");
                execute(build_args, ctx);
                if (fs::exists(repo_root / "build" / "bin" / "hacbrewpack")) {
                    hbp_bin = repo_root / "build" / "bin" / "hacbrewpack";
                }
            }

            if (!hbp_bin.empty() && fs::exists(hbp_bin)) {
                fs::copy_file(hbp_bin, inner_stage / "bin" / "hacbrewpack", fs::copy_options::overwrite_existing);
                fs::permissions(inner_stage / "bin" / "hacbrewpack",
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec);
            }

            // 1. Copy Headers
            for (const auto& entry : fs::directory_iterator(repo_root / "sdk" / "core" / "include" / "nxdev")) {
                if (entry.is_regular_file()) {
                    fs::copy_file(entry.path(), inner_stage / "include" / "nxdev" / entry.path().filename(), fs::copy_options::overwrite_existing);
                }
            }
            for (const auto& mod : fs::directory_iterator(repo_root / "sdk" / "modules")) {
                if (mod.is_directory() && fs::exists(mod.path() / "include" / "nxdev")) {
                    for (const auto& h : fs::directory_iterator(mod.path() / "include" / "nxdev")) {
                        if (h.is_regular_file()) {
                            fs::copy_file(h.path(), inner_stage / "include" / "nxdev" / h.path().filename(), fs::copy_options::overwrite_existing);
                        }
                    }
                }
            }

            // 2. Copy Sources
            for (const auto& entry : fs::directory_iterator(repo_root / "sdk" / "core" / "src")) {
                if (entry.is_regular_file()) {
                    fs::copy_file(entry.path(), inner_stage / "src" / "core" / entry.path().filename(), fs::copy_options::overwrite_existing);
                }
            }
            for (const auto& mod : fs::directory_iterator(repo_root / "sdk" / "modules")) {
                if (mod.is_directory() && fs::exists(mod.path() / "src")) {
                    for (const auto& s : fs::directory_iterator(mod.path() / "src")) {
                        if (s.is_regular_file()) {
                            fs::copy_file(s.path(), inner_stage / "src" / "modules" / s.path().filename(), fs::copy_options::overwrite_existing);
                        }
                    }
                }
            }

            // 3. Copy CMake Modules
            for (const auto& entry : fs::directory_iterator(repo_root / "cmake")) {
                if (entry.is_regular_file() && entry.path().extension() == ".cmake") {
                    fs::copy_file(entry.path(), inner_stage / "share" / "nxdev" / "cmake" / entry.path().filename(), fs::copy_options::overwrite_existing);
                }
            }
            if (fs::exists(repo_root / "cmake" / "toolchains" / "NXDevSwitch.cmake")) {
                fs::copy_file(repo_root / "cmake" / "toolchains" / "NXDevSwitch.cmake", inner_stage / "share" / "nxdev" / "cmake" / "toolchains" / "NXDevSwitch.cmake", fs::copy_options::overwrite_existing);
            }

            // 4. Copy Package Registry
            if (fs::exists(repo_root / "package-registry" / "registry.json")) {
                fs::copy_file(repo_root / "package-registry" / "registry.json", inner_stage / "share" / "nxdev" / "package-registry" / "registry.json", fs::copy_options::overwrite_existing);
            }

            // 5. Copy Templates
            if (fs::exists(repo_root / "templates")) {
                fs::copy(repo_root / "templates", inner_stage / "share" / "nxdev" / "templates", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }

            // 6. Copy Licenses
            if (fs::exists(repo_root / "LICENSE")) {
                fs::copy_file(repo_root / "LICENSE", inner_stage / "licenses" / "LICENSE", fs::copy_options::overwrite_existing);
            }
            if (fs::exists(repo_root / "THIRD_PARTY_NOTICES.md")) {
                fs::copy_file(repo_root / "THIRD_PARTY_NOTICES.md", inner_stage / "licenses" / "THIRD_PARTY_NOTICES.md", fs::copy_options::overwrite_existing);
            }

            // 7. Write sdk.json version metadata
            {
                std::ofstream jf(inner_stage / "share" / "nxdev" / "sdk.json");
                jf << "{\n"
                   << "  \"name\": \"NXDevSDK\",\n"
                   << "  \"version\": \"" << ctx.env.sdk().version << "\",\n"
                   << "  \"layoutVersion\": 1,\n"
                   << "  \"platform\": \"switch\",\n"
                   << "  \"minDevkitProVersion\": \"r20\",\n"
                   << "  \"minLibnxVersion\": \"4.5.0\"\n"
                   << "}\n";
            }

            // 8. Package SDK.zip
            fs::path inner_zip = outer_stage / "SDK.zip";
            std::string zip_cmd = "cmake -E tar cf \"" + inner_zip.string() + "\" --format=zip -- *";
            std::string full_cmd = "cd \"" + inner_stage.string() + "\" && " + zip_cmd;
            int res = std::system(full_cmd.c_str());
            if (res != 0) {
                std::cerr << c_red("Error: ") << "Failed to create SDK.zip (returned " << res << ")\n";
                return 1;
            }

            // 9. Write install.sh
            {
                fs::path inst_path = outer_stage / "install.sh";
                std::ofstream inf(inst_path);
                inf << INSTALL_SH_CONTENT;
                fs::permissions(inst_path, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec);
            }

            // 10. Package outer NXDevSDK.zip
            fs::path final_zip = out_dir / "NXDevSDK.zip";
            std::string outer_zip_cmd = "cmake -E tar cf \"" + final_zip.string() + "\" --format=zip -- *";
            std::string full_outer_cmd = "cd \"" + outer_stage.string() + "\" && " + outer_zip_cmd;
            res = std::system(full_outer_cmd.c_str());
            if (res != 0) {
                std::cerr << c_red("Error: ") << "Failed to create NXDevSDK.zip (returned " << res << ")\n";
                return 1;
            }

            // 11. Clean staging directory
            fs::remove_all(stage_dir);

            if (json_output) {
                std::cout << "{\n"
                          << "  \"status\": \"success\",\n"
                          << "  \"archive\": \"" << final_zip.string() << "\",\n"
                          << "  \"version\": \"" << ctx.env.sdk().version << "\",\n"
                          << "  \"sizeBytes\": " << fs::file_size(final_zip) << "\n"
                          << "}\n";
            } else {
                std::cout << c_green("✓") << c_bold(" Built NXDevSDK distribution archive:") << "\n"
                          << "  Archive:    " << c_cyan(final_zip.string()) << "\n"
                          << "  Version:    " << ctx.env.sdk().version << "\n"
                          << "  Size:       " << (fs::file_size(final_zip) / 1024) << " KB\n\n"
                          << "Installation on target machines:\n"
                          << "  $ unzip NXDevSDK.zip\n"
                          << "  $ sudo ./install.sh\n";
            }
            return 0;

        } catch (const std::exception& e) {
            std::cerr << c_red("Error: ") << "SDK packaging failed: " << e.what() << "\n";
            return 1;
        }
    }

    if (subcmd == "build-hacbrewpack") {
        size_t max_memory = 0;
        uint32_t jobs = 1;
        bool unsafe = false;
        bool verbose = ctx.verbose;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--max-memory" && i + 1 < args.size()) {
                auto parsed = exec::parse_memory_size_string(args[++i]);
                if (!parsed.has_value()) {
                    std::cerr << "Error: Invalid --max-memory value '" << args[i] << "'. Examples: 512M, 2G, 4096M, auto, unlimited.\n";
                    return 1;
                }
                max_memory = *parsed;
            } else if ((args[i] == "--jobs" || args[i] == "-j") && i + 1 < args.size()) {
                jobs = std::stoul(args[++i]);
            } else if (args[i] == "--verbose" || args[i] == "-v") {
                verbose = true;
            } else if (args[i] == "--unsafe-no-resource-limits") {
                unsafe = true;
            }
        }

        // Locate repository root
        fs::path repo_root;
        fs::path p = fs::current_path();
        for (int i = 0; i < 4 && p != p.root_path(); ++i) {
            if (fs::exists(p / "third_party" / "hacbrewpack" / "CMakeLists.txt")) {
                repo_root = p;
                break;
            }
            p = p.parent_path();
        }

        if (repo_root.empty()) {
            std::cerr << c_red("Error: ") << "Cannot locate third_party/hacbrewpack source tree.\n";
            return 1;
        }

        exec::MemoryInfo mem = exec::ResourceCalculator::detect_memory();
        size_t reserve = exec::ResourceCalculator::compute_host_reserve(mem);
        size_t budget = exec::ResourceCalculator::compute_safe_memory_budget(
            exec::WorkloadType::BuildThirdPartyBackend,
            mem,
            max_memory
        );

        if (budget < exec::ResourceCalculator::MINIMUM_VIABLE_BUILD_BUDGET && !unsafe) {
            std::cerr << c_red("Error: ") << "Not enough safe memory is currently available to build hacBrewPack.\n"
                      << "  Available: " << exec::format_bytes(mem.available_bytes) << "\n"
                      << "  Reserved:  " << exec::format_bytes(reserve) << "\n"
                      << "  Safe build budget: " << exec::format_bytes(budget) << "\n\n"
                      << "NXDev will not start the backend compiler because it could destabilize WSL.\n";
            return 1;
        }

        bool was_clamped = false;
        uint32_t effective_jobs = exec::ResourceCalculator::compute_safe_job_count(
            exec::WorkloadType::BuildThirdPartyBackend,
            mem,
            budget,
            jobs,
            unsafe,
            &was_clamped
        );

        exec::ResourceCalculator::sanitize_environment(effective_jobs);

        if (verbose) {
            std::cout << "Build resource policy:\n"
                      << "  Platform:           " << (mem.is_wsl ? "WSL2" : "Linux / Host") << "\n"
                      << "  CPUs visible:       " << std::thread::hardware_concurrency() << "\n"
                      << "  Memory visible:     " << exec::format_bytes(mem.total_bytes) << "\n"
                      << "  Memory available:   " << exec::format_bytes(mem.available_bytes) << "\n"
                      << "  Reserved:           " << exec::format_bytes(reserve) << "\n"
                      << "  Build memory limit: " << exec::format_bytes(budget) << "\n"
                      << "  Compile jobs:       " << effective_jobs << "\n"
                      << "  Link jobs:          1\n\n";
        }

        fs::path build_dir = repo_root / "build";
        fs::create_directories(build_dir);

        // 1. Configure
        std::cout << "==> Configuring hacBrewPack backend build...\n";
        std::vector<std::string> cfg_args = {
            "-S", repo_root.string(),
            "-B", build_dir.string(),
            "-DCMAKE_BUILD_TYPE=Release"
        };
        auto cfg_res = exec::ProcessExecutor::execute("cmake", cfg_args, 60000, repo_root.string());
        if (!cfg_res.success) {
            std::cerr << c_red("Error: ") << "CMake configuration failed for hacBrewPack: " << cfg_res.stderr_output << "\n";
            return 1;
        }

        // 2. Build with resource limits
        std::cout << "==> Compiling hacBrewPack (jobs=" << effective_jobs << ", memory limit=" << exec::format_bytes(budget) << ")...\n";
        exec::ProcessResourcePolicy policy;
        policy.workload = exec::WorkloadType::BuildThirdPartyBackend;
        policy.max_memory_bytes = budget;
        policy.min_host_memory_reserve_bytes = reserve;
        policy.timeout_ms = 300000;
        policy.working_dir = repo_root.string();
        policy.unsafe_no_limits = unsafe;

        std::vector<std::string> bld_args = {
            "--build", build_dir.string(),
            "--target", "nxdev_hacbrewpack",
            "--parallel", std::to_string(effective_jobs)
        };

        auto bld_res = exec::ControlledProcessRunner::execute("cmake", bld_args, policy);
        if (!bld_res.success) {
            if (bld_res.resource_limit_exceeded) {
                std::cerr << "\n" << c_red("hacBrewPack backend build stopped by NXDev.") << "\n\n"
                          << "Reason:\n  " << exec::resource_limit_reason_to_string(bld_res.resource_limit_reason) << "\n\n"
                          << "Peak RSS:\n  " << exec::format_bytes(bld_res.peak_rss_bytes) << "\n\n"
                          << "Configured limit:\n  " << exec::format_bytes(budget) << "\n\n"
                          << "This build was terminated before it could exhaust WSL memory.\n";
            } else {
                std::cerr << c_red("Error: ") << "hacBrewPack build failed with exit code " << bld_res.exit_code << "\n"
                          << bld_res.stderr_output << "\n";
            }
            return 1;
        }

        std::cout << c_green("✓") << " hacBrewPack built successfully! (Peak RSS: " << exec::format_bytes(bld_res.peak_rss_bytes)
                  << ", Duration: " << bld_res.duration_ms << " ms)\n";
        return 0;
    }

    std::cerr << "Error: Unknown sdk subcommand '" << subcmd << "'. See 'nxdev sdk --help'.\n";
    return 1;
}

} // namespace nxdev::cli
