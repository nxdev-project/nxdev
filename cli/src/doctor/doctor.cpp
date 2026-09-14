#include <nxdev/doctor/doctor.hpp>
#include <nxdev/packages/registry.hpp>
#include <nxdev/packages/package_manager.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/nsp_backend.hpp>
#include <nxdev/device/device_manager.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <nxdev/exec/controlled_runner.hpp>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace nxdev::doctor {

std::string CheckItem::status_string() const {
    switch (status) {
        case CheckStatus::Pass: return "pass";
        case CheckStatus::Warning: return "warning";
        case CheckStatus::Error: return "error";
        case CheckStatus::Info: return "info";
        case CheckStatus::Skipped: return "skipped";
    }
    return "unknown";
}

std::string CheckItem::symbol() const {
    switch (status) {
        case CheckStatus::Pass: return "[✓]";
        case CheckStatus::Warning: return "[!]";
        case CheckStatus::Error: return "[✗]";
        case CheckStatus::Info: return "[i]";
        case CheckStatus::Skipped: return "[-]";
    }
    return "[?]";
}

void Doctor::add_check(CheckItem item) {
    if (item.status == CheckStatus::Pass) pass_count_++;
    else if (item.status == CheckStatus::Warning) warning_count_++;
    else if (item.status == CheckStatus::Error) error_count_++;
    else if (item.status == CheckStatus::Info) info_count_++;

    checks_.push_back(std::move(item));
}

void Doctor::run_diagnostics(
    const env::Environment& env,
    const std::optional<project::NXDevProject>& project,
    DoctorProfile profile
) {
    checks_.clear();
    pass_count_ = warning_count_ = error_count_ = info_count_ = 0;

    // 1. Host Environment Check
    {
        std::string host_msg = "Host OS: " + env.host().os_name() + " (" + env.host().arch_name() + ")";
        add_check(CheckItem{
            .id = "host.platform",
            .category = "Host Environment",
            .status = (env.host().os != env::OperatingSystem::Unknown) ? CheckStatus::Pass : CheckStatus::Warning,
            .message = host_msg,
            .path = "",
            .remedy_suggestion = ""
        });
    }

    // 2. WSL Integration
    if (env.host().is_wsl) {
        add_check(CheckItem{
            .id = "host.wsl",
            .category = "WSL Integration",
            .status = CheckStatus::Pass,
            .message = "Running inside " + env.host().wsl_version_name() + " (" + env.host().wsl_distro_name + ")",
            .path = "",
            .remedy_suggestion = ""
        });

        if (project.has_value() && env.host().is_mounted_windows_path(project->root_path())) {
            add_check(CheckItem{
                .id = "host.wsl.filesystem",
                .category = "WSL Integration",
                .status = CheckStatus::Warning,
                .message = "Project is located on a Windows-mounted filesystem (" + project->root_path() + ")",
                .path = project->root_path(),
                .remedy_suggestion = "For optimal build performance and I/O speed, consider placing projects inside the native WSL ext4 filesystem (e.g. ~/projects/)."
            });
        }
    }

    // 2.1. Host / WSL Resources Diagnostics
    {
        std::string res_cat = env.host().is_wsl ? "WSL Resources" : "Host Resources";
        exec::MemoryInfo mem = exec::ResourceCalculator::detect_memory();
        exec::ControllerType ctl = exec::ControlledProcessRunner::detect_best_controller();
        size_t budget = exec::ResourceCalculator::compute_safe_memory_budget(exec::WorkloadType::BuildApplication, mem);
        uint32_t app_jobs = exec::ResourceCalculator::compute_safe_job_count(exec::WorkloadType::BuildApplication, mem, budget);

        add_check(CheckItem{
            .id = "resources.memory.visible",
            .category = res_cat,
            .status = CheckStatus::Pass,
            .message = "Visible memory: " + exec::format_bytes(mem.total_bytes),
            .path = "",
            .remedy_suggestion = ""
        });

        add_check(CheckItem{
            .id = "resources.memory.available",
            .category = res_cat,
            .status = (mem.available_bytes >= 1024ULL * 1024 * 1024) ? CheckStatus::Pass : CheckStatus::Warning,
            .message = "Available memory: " + exec::format_bytes(mem.available_bytes),
            .path = "",
            .remedy_suggestion = (mem.available_bytes < 1024ULL * 1024 * 1024) ? "Available memory is low. Close memory-intensive tasks before starting large builds." : ""
        });

        add_check(CheckItem{
            .id = "resources.controller",
            .category = res_cat,
            .status = CheckStatus::Pass,
            .message = "Resource controller: " + exec::controller_type_to_string(ctl),
            .path = "",
            .remedy_suggestion = ""
        });

        add_check(CheckItem{
            .id = "resources.budget",
            .category = res_cat,
            .status = CheckStatus::Info,
            .message = "Default build memory ceiling: " + exec::format_bytes(budget),
            .path = "",
            .remedy_suggestion = ""
        });

        add_check(CheckItem{
            .id = "resources.jobs.app",
            .category = res_cat,
            .status = CheckStatus::Info,
            .message = "Default application build jobs: " + std::to_string(app_jobs),
            .path = "",
            .remedy_suggestion = ""
        });

        add_check(CheckItem{
            .id = "resources.jobs.hacbrewpack",
            .category = res_cat,
            .status = CheckStatus::Info,
            .message = "hacBrewPack build jobs: 1",
            .path = "",
            .remedy_suggestion = ""
        });
    }

    // 3. Project & Manifest Checks
    if (project.has_value()) {
        add_check(CheckItem{
            .id = "project.found",
            .category = "NXDev Project",
            .status = CheckStatus::Pass,
            .message = "Project discovered at: " + project->root_path(),
            .path = project->root_path(),
            .remedy_suggestion = ""
        });

        if (project->is_valid()) {
            add_check(CheckItem{
                .id = "manifest.valid",
                .category = "NXDevAppManifest",
                .status = CheckStatus::Pass,
                .message = "Manifest 'nxapp.yaml' is valid (App: " + project->manifest().application().name + " v" + project->manifest().application().version + ")",
                .path = project->manifest_path(),
                .remedy_suggestion = ""
            });

            // Project Dependencies Check
            auto reg_res = packages::PackageRegistry::load_default(env);
            if (reg_res.is_success()) {
                const auto& reg = reg_res.value();
                packages::PackageManager pm(env, reg);
                const auto& deps = project->manifest().dependencies();

                if (deps.empty()) {
                    add_check(CheckItem{
                        .id = "dependencies.empty",
                        .category = "Project Dependencies",
                        .status = CheckStatus::Pass,
                        .message = "No additional dependencies declared (using standard NXDev/libnx core)",
                        .path = "",
                        .remedy_suggestion = ""
                    });
                } else {
                    auto statuses = pm.check_manifest_dependencies(project->manifest());
                    for (const auto& s : statuses) {
                        if (s.status == packages::PackageStatus::Installed) {
                            std::string detail = s.definition.is_builtin() ? "built-in" : (s.definition.devkitpro.packages.empty() ? "installed" : s.definition.devkitpro.packages[0] + " installed");
                            if (!s.detected_version.empty() && s.detected_version != "built-in") {
                                detail += " (" + s.detected_version + ")";
                            }
                            add_check(CheckItem{
                                .id = "dependency." + s.definition.id,
                                .category = "Project Dependencies",
                                .status = CheckStatus::Pass,
                                .message = s.definition.id + " — " + detail,
                                .path = "",
                                .remedy_suggestion = ""
                            });
                        } else if (s.status == packages::PackageStatus::Missing) {
                            std::string req = s.missing_system_packages.empty() ? "" : s.missing_system_packages[0];
                            add_check(CheckItem{
                                .id = "dependency." + s.definition.id,
                                .category = "Project Dependencies",
                                .status = CheckStatus::Warning,
                                .message = s.definition.id + " — missing " + (req.empty() ? "package" : "(" + req + ")"),
                                .path = "",
                                .remedy_suggestion = "Run 'nxdev package install " + s.definition.id + "' or 'nxdev package install --missing'"
                            });
                        } else if (s.status == packages::PackageStatus::Incomplete) {
                            add_check(CheckItem{
                                .id = "dependency." + s.definition.id,
                                .category = "Project Dependencies",
                                .status = CheckStatus::Error,
                                .message = s.definition.id + " — incomplete installation (missing headers or libraries)",
                                .path = "",
                                .remedy_suggestion = "Reinstall with 'nxdev package install " + s.definition.id + "'"
                            });
                        } else {
                            add_check(CheckItem{
                                .id = "dependency." + s.definition.id,
                                .category = "Project Dependencies",
                                .status = CheckStatus::Warning,
                                .message = s.definition.id + " — status unknown or unsupported on host",
                                .path = "",
                                .remedy_suggestion = "Verify portlibs under devkitPro tree."
                            });
                        }
                    }
                }
            }
        } else {
            add_check(CheckItem{
                .id = "manifest.invalid",
                .category = "NXDevAppManifest",
                .status = CheckStatus::Error,
                .message = "Manifest 'nxapp.yaml' has " + std::to_string(project->diagnostics().error_count()) + " validation errors",
                .path = project->manifest_path(),
                .remedy_suggestion = "Run 'nxdev manifest validate' to inspect and fix manifest issues."
            });
        }
    } else {
        add_check(CheckItem{
            .id = "project.not_found",
            .category = "NXDev Project",
            .status = CheckStatus::Info,
            .message = "No active NXDev project found in current directory or parent tree",
            .path = "",
            .remedy_suggestion = "Run 'nxdev new <name>' or execute commands from within an existing project directory containing nxapp.yaml."
        });
    }

    // 4. NXDevSDK Discovery
    {
        if (env.sdk().is_valid) {
            // Check 1: NXDEV_SDK_ROOT environment variable
            if (env.sdk().env_root_set) {
                add_check(CheckItem{
                    .id = "sdk.nxdev.env",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Pass,
                    .message = "NXDEV_SDK_ROOT=" + env.sdk().path,
                    .path = env.sdk().path,
                    .remedy_suggestion = ""
                });
            } else if (env.sdk().source == env::SdkSource::StandardInstallPath) {
                add_check(CheckItem{
                    .id = "sdk.nxdev.env",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Pass,
                    .message = "Standard installation path detected (" + env.sdk().path + ")",
                    .path = env.sdk().path,
                    .remedy_suggestion = ""
                });
            } else if (env.sdk().source == env::SdkSource::SourceTree) {
                add_check(CheckItem{
                    .id = "sdk.nxdev.env",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Pass,
                    .message = "Source tree layout detected (" + env.sdk().path + ")",
                    .path = env.sdk().path,
                    .remedy_suggestion = ""
                });
            } else {
                add_check(CheckItem{
                    .id = "sdk.nxdev.env",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Warning,
                    .message = "NXDEV_SDK_ROOT is not set",
                    .path = "",
                    .remedy_suggestion = "Export NXDEV_SDK_ROOT=" + env.sdk().path + " in your shell profile."
                });
            }

            // Check 2: SDK root existence
            add_check(CheckItem{
                .id = "sdk.nxdev.root",
                .category = "NXDevSDK",
                .status = CheckStatus::Pass,
                .message = "SDK root exists (" + env.sdk().path + ")",
                .path = env.sdk().path,
                .remedy_suggestion = ""
            });

            // Check 3: $NXDEV_SDK_ROOT/bin in PATH
            if (env.sdk().is_installed) {
                if (env.sdk().bin_in_path) {
                    add_check(CheckItem{
                        .id = "sdk.nxdev.path",
                        .category = "NXDevSDK",
                        .status = CheckStatus::Pass,
                        .message = env.sdk().path + "/bin is on PATH",
                        .path = env.sdk().path + "/bin",
                        .remedy_suggestion = ""
                    });
                } else {
                    add_check(CheckItem{
                        .id = "sdk.nxdev.path",
                        .category = "NXDevSDK",
                        .status = CheckStatus::Warning,
                        .message = env.sdk().path + "/bin is not on system PATH",
                        .path = env.sdk().path + "/bin",
                        .remedy_suggestion = "Add 'export PATH=\"" + env.sdk().path + "/bin:$PATH\"' to your ~/.bashrc or ~/.profile."
                    });
                }
            }

            // Check 4: CMake package availability
            if (env.sdk().has_cmake_package) {
                add_check(CheckItem{
                    .id = "sdk.nxdev.cmake",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Pass,
                    .message = "CMake package found",
                    .path = env.sdk().path,
                    .remedy_suggestion = ""
                });
            } else {
                add_check(CheckItem{
                    .id = "sdk.nxdev.cmake",
                    .category = "NXDevSDK",
                    .status = CheckStatus::Warning,
                    .message = "NXDevConfig.cmake not found in SDK tree",
                    .path = env.sdk().path,
                    .remedy_suggestion = "Reinstall NXDevSDK or verify share/nxdev/cmake contents."
                });
            }

            // Check 5: SDK version
            add_check(CheckItem{
                .id = "sdk.nxdev.version",
                .category = "NXDevSDK",
                .status = CheckStatus::Pass,
                .message = "SDK version " + env.sdk().version,
                .path = env.sdk().path,
                .remedy_suggestion = ""
            });

        } else if (!env.sdk().path.empty()) {
            add_check(CheckItem{
                .id = "sdk.nxdev.root",
                .category = "NXDevSDK",
                .status = CheckStatus::Warning,
                .message = "NXDevSDK path configured but invalid or incomplete: " + env.sdk().path,
                .path = env.sdk().path,
                .remedy_suggestion = "Verify NXDEV_SDK_ROOT or reinstall NXDevSDK via NXDevSDK.zip."
            });
        } else {
            add_check(CheckItem{
                .id = "sdk.nxdev.root",
                .category = "NXDevSDK",
                .status = CheckStatus::Info,
                .message = "NXDevSDK not detected in /opt/nxdev (will use project/bundled module dependencies)",
                .path = "",
                .remedy_suggestion = "To install globally, extract NXDevSDK.zip and run ./install.sh."
            });
        }
    }

    // 5. devkitPro Discovery
    if (profile == DoctorProfile::All || profile == DoctorProfile::Build || profile == DoctorProfile::Pack) {
        if (!env.devkitpro().is_configured) {
            add_check(CheckItem{
                .id = "toolchain.devkitpro",
                .category = "devkitPro",
                .status = CheckStatus::Error,
                .message = "DEVKITPRO is not configured or found",
                .path = "",
                .remedy_suggestion = "Run 'nxdev prepare' or set 'export DEVKITPRO=/opt/devkitpro' in your shell profile."
            });
        } else if (!env.devkitpro().is_valid) {
            add_check(CheckItem{
                .id = "toolchain.devkitpro",
                .category = "devkitPro",
                .status = CheckStatus::Error,
                .message = "Configured devkitPro path does not exist or is invalid: " + env.devkitpro().path,
                .path = env.devkitpro().path,
                .remedy_suggestion = "Verify your DEVKITPRO environment variable or reinstall via 'nxdev prepare --repair'."
            });
        } else {
            add_check(CheckItem{
                .id = "toolchain.devkitpro",
                .category = "devkitPro",
                .status = CheckStatus::Pass,
                .message = "DEVKITPRO=" + env.devkitpro().path + " (source: " + env.devkitpro().source_name() + ")",
                .path = env.devkitpro().path,
                .remedy_suggestion = ""
            });

            if (!env.devkitpro().env_var_set) {
                add_check(CheckItem{
                    .id = "toolchain.devkitpro.env",
                    .category = "devkitPro",
                    .status = CheckStatus::Warning,
                    .message = "DEVKITPRO environment variable is not explicitly set (using detected tree at " + env.devkitpro().path + ")",
                    .path = env.devkitpro().path,
                    .remedy_suggestion = "Export 'DEVKITPRO=" + env.devkitpro().path + "' in your ~/.bashrc or ~/.profile."
                });
            }

            if (!env.devkitpro().bin_in_path && env.switch_tools().is_found) {
                add_check(CheckItem{
                    .id = "toolchain.devkitpro.tools_path",
                    .category = "devkitPro",
                    .status = CheckStatus::Warning,
                    .message = "devkitPro tools bin directory is not available in the current PATH (" + env.devkitpro().path + "/tools/bin)",
                    .path = env.devkitpro().path + "/tools/bin",
                    .remedy_suggestion = "Add 'export PATH=\"" + env.devkitpro().path + "/tools/bin:$PATH\"' to your ~/.bashrc or ~/.profile."
                });
            }
        }
    }

    // 6. devkitA64 Discovery
    if (profile == DoctorProfile::All || profile == DoctorProfile::Build) {
        if (!env.devkita64().is_configured) {
            add_check(CheckItem{
                .id = "toolchain.devkita64",
                .category = "devkitA64",
                .status = CheckStatus::Error,
                .message = "devkitA64 is not configured or found",
                .path = "",
                .remedy_suggestion = "Install devkitA64 via 'nxdev prepare' or set 'export DEVKITA64=$DEVKITPRO/devkitA64'."
            });
        } else if (!env.devkita64().is_valid) {
            add_check(CheckItem{
                .id = "toolchain.devkita64",
                .category = "devkitA64",
                .status = CheckStatus::Error,
                .message = "Configured devkitA64 path does not exist: " + env.devkita64().path,
                .path = env.devkita64().path,
                .remedy_suggestion = "Install devkitA64 via 'nxdev prepare --repair' or update DEVKITA64."
            });
        } else if (!env.devkita64().is_consistent) {
            add_check(CheckItem{
                .id = "toolchain.devkita64",
                .category = "devkitA64",
                .status = CheckStatus::Error,
                .message = "DEVKITA64 (" + env.devkita64().path + ") is inconsistent with DEVKITPRO (" + env.devkitpro().path + ")",
                .path = env.devkita64().path,
                .remedy_suggestion = "Set 'DEVKITA64=" + env.devkitpro().path + "/devkitA64' or run 'nxdev prepare --repair'."
            });
        } else {
            add_check(CheckItem{
                .id = "toolchain.devkita64",
                .category = "devkitA64",
                .status = CheckStatus::Pass,
                .message = "DEVKITA64=" + env.devkita64().path,
                .path = env.devkita64().path,
                .remedy_suggestion = ""
            });

            if (!env.devkita64().env_var_set) {
                add_check(CheckItem{
                    .id = "toolchain.devkita64.env",
                    .category = "devkitA64",
                    .status = CheckStatus::Warning,
                    .message = "DEVKITA64 environment variable is not explicitly set (using detected tree at " + env.devkita64().path + ")",
                    .path = env.devkita64().path,
                    .remedy_suggestion = "Export 'DEVKITA64=" + env.devkita64().path + "' in your ~/.bashrc or ~/.profile."
                });
            }

            if (!env.devkita64().bin_in_path) {
                add_check(CheckItem{
                    .id = "toolchain.devkita64.path",
                    .category = "devkitA64",
                    .status = CheckStatus::Warning,
                    .message = "devkitA64 bin directory is not available in the current PATH (" + env.devkita64().path + "/bin)",
                    .path = env.devkita64().path + "/bin",
                    .remedy_suggestion = "Add 'export PATH=\"" + env.devkita64().path + "/bin:$PATH\"' to your ~/.bashrc or ~/.profile."
                });
            }
        }
    }

    // 6. Compiler Toolchain Checks
    if (profile == DoctorProfile::All || profile == DoctorProfile::Build) {
        const auto* gcc = env.get_tool("aarch64-none-elf-gcc");
        if (gcc && gcc->usable) {
            add_check(CheckItem{
                .id = "compiler.gcc",
                .category = "Compiler Toolchain",
                .status = CheckStatus::Pass,
                .message = "AArch64 GCC found (" + gcc->version + ")",
                .path = gcc->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "compiler.gcc",
                .category = "Compiler Toolchain",
                .status = CheckStatus::Error,
                .message = "AArch64 GCC (aarch64-none-elf-gcc) not found or not executable",
                .path = gcc ? gcc->path : "",
                .remedy_suggestion = "Ensure devkitA64 is properly installed and its binaries are available."
            });
        }

        const auto* gxx = env.get_tool("aarch64-none-elf-g++");
        if (gxx && gxx->usable) {
            add_check(CheckItem{
                .id = "compiler.gxx",
                .category = "Compiler Toolchain",
                .status = CheckStatus::Pass,
                .message = "AArch64 G++ found (" + gxx->version + ")",
                .path = gxx->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "compiler.gxx",
                .category = "Compiler Toolchain",
                .status = CheckStatus::Error,
                .message = "AArch64 G++ (aarch64-none-elf-g++) not found or not executable",
                .path = gxx ? gxx->path : "",
                .remedy_suggestion = "Ensure devkitA64 C++ compiler package is installed."
            });
        }
    }

    // 7. libnx Detection
    if (profile == DoctorProfile::All || profile == DoctorProfile::Build) {
        if (env.libnx().found) {
            add_check(CheckItem{
                .id = "library.libnx",
                .category = "libnx",
                .status = CheckStatus::Pass,
                .message = "libnx system library found (headers: " + env.libnx().include_path + ")",
                .path = env.libnx().library_path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "library.libnx",
                .category = "libnx",
                .status = CheckStatus::Error,
                .message = "libnx library not found under devkitPro tree",
                .path = "",
                .remedy_suggestion = "Install libnx via 'dkp-pacman -S libnx'."
            });
        }
    }

    // 8. switch-tools / Packaging Tools
    if (profile == DoctorProfile::All || profile == DoctorProfile::Pack) {
        const auto* elf2nro = env.get_tool("elf2nro");
        if (elf2nro && elf2nro->usable) {
            add_check(CheckItem{
                .id = "tool.elf2nro",
                .category = "Packaging Tools",
                .status = CheckStatus::Pass,
                .message = "elf2nro found (" + elf2nro->path + ")",
                .path = elf2nro->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.elf2nro",
                .category = "Packaging Tools",
                .status = CheckStatus::Error,
                .message = "elf2nro not found",
                .path = "",
                .remedy_suggestion = "Install switch-tools via 'dkp-pacman -S switch-tools'."
            });
        }

        const auto* nacptool = env.get_tool("nacptool");
        if (nacptool && nacptool->usable) {
            add_check(CheckItem{
                .id = "tool.nacptool",
                .category = "Packaging Tools",
                .status = CheckStatus::Pass,
                .message = "nacptool found (" + nacptool->path + ")",
                .path = nacptool->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.nacptool",
                .category = "Packaging Tools",
                .status = CheckStatus::Error,
                .message = "nacptool not found",
                .path = "",
                .remedy_suggestion = "Install switch-tools via 'dkp-pacman -S switch-tools'."
            });
        }

        const auto* elf2nso = env.get_tool("elf2nso");
        if (elf2nso && elf2nso->usable) {
            add_check(CheckItem{
                .id = "tool.elf2nso",
                .category = "Packaging Tools",
                .status = CheckStatus::Pass,
                .message = "elf2nso found (" + elf2nso->path + ")",
                .path = elf2nso->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.elf2nso",
                .category = "Packaging Tools",
                .status = CheckStatus::Warning,
                .message = "elf2nso not found (required for NSP packaging)",
                .path = "",
                .remedy_suggestion = "Install switch-tools via 'dkp-pacman -S switch-tools'."
            });
        }

        const auto* npdmtool = env.get_tool("npdmtool");
        if (npdmtool && npdmtool->usable) {
            add_check(CheckItem{
                .id = "tool.npdmtool",
                .category = "Packaging Tools",
                .status = CheckStatus::Pass,
                .message = "npdmtool found (" + npdmtool->path + ")",
                .path = npdmtool->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.npdmtool",
                .category = "Packaging Tools",
                .status = CheckStatus::Warning,
                .message = "npdmtool not found (required for NSP packaging)",
                .path = "",
                .remedy_suggestion = "Install switch-tools via 'dkp-pacman -S switch-tools'."
            });
        }

        const auto* hacbrewpack = env.get_tool("hacbrewpack");
        std::string hbp_path;
        if (hacbrewpack && hacbrewpack->usable) {
            hbp_path = hacbrewpack->path;
        } else {
            // Check build/bin/hacbrewpack or standard locations
            std::vector<std::string> candidates = {
                "./build/bin/hacbrewpack",
                "/usr/local/bin/hacbrewpack",
                "/opt/nxdev/bin/hacbrewpack"
            };
            for (const auto& c : candidates) {
                if (fs::exists(c)) {
                    hbp_path = c;
                    break;
                }
            }
        }

        if (!hbp_path.empty()) {
            bool is_wsl = env.host().is_wsl;
            std::string arch_info = is_wsl ? "native Linux backend (WSL environment)" : "native Linux backend";
            add_check(CheckItem{
                .id = "tool.hacbrewpack",
                .category = "Packaging Tools",
                .status = CheckStatus::Pass,
                .message = "gayhearts/hacBrewPack backend built (rev " + std::string(pack::HacBrewPackAdapter::PINNED_BACKEND_REVISION).substr(0, 7) + ", " + arch_info + " at " + hbp_path + ")",
                .path = hbp_path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.hacbrewpack",
                .category = "Packaging Tools",
                .status = CheckStatus::Info,
                .message = "gayhearts/hacBrewPack not detected in PATH (will use bundled NXDevThirdParty::hacBrewPack)",
                .path = "",
                .remedy_suggestion = "Build the host tool target 'nxdev_hacbrewpack'."
            });
        }

        // Switch keys resolution check
        pack::PackageRequest test_req;
        if (project.has_value() && project->is_valid()) {
            test_req.project_root = project->root_path();
        }
        std::string resolved_keys = pack::HacBrewPackAdapter::resolve_keys_file(test_req, &env);
        if (!resolved_keys.empty() && fs::exists(resolved_keys)) {
            add_check(CheckItem{
                .id = "pack.keys",
                .category = "NSP Packaging",
                .status = CheckStatus::Pass,
                .message = "User Switch keys file found (" + resolved_keys + ")",
                .path = resolved_keys,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "pack.keys",
                .category = "NSP Packaging",
                .status = CheckStatus::Info,
                .message = "No Switch keys file (prod.keys) configured (required for NSP packaging)",
                .path = "",
                .remedy_suggestion = "Configure via NXDEV_KEYS env var, nxdev pack nsp --keys <path>, or place prod.keys in ~/.switch/"
            });
        }

        // Default icon detection
        std::string dkp_root = env.devkitpro().path;
        if (dkp_root.empty()) dkp_root = "/opt/devkitpro";
        std::string def_icon = dkp_root + "/libnx/default_icon.jpg";
        if (fs::exists(def_icon)) {
            add_check(CheckItem{
                .id = "pack.default_icon",
                .category = "Packaging Assets",
                .status = CheckStatus::Pass,
                .message = "libnx default icon found (" + def_icon + ")",
                .path = def_icon,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "pack.default_icon",
                .category = "Packaging Assets",
                .status = CheckStatus::Warning,
                .message = "libnx default icon not found at " + def_icon,
                .path = "",
                .remedy_suggestion = "Ensure libnx package is installed properly."
            });
        }

        // Project asset validation (if project active)
        if (project.has_value() && project->is_valid()) {
            const auto& mf = project->manifest();
            const auto& icon_cfg = mf.assets().icon;
            if (icon_cfg.type == manifest::IconSourceType::ProjectFile && !icon_cfg.raw_path.empty()) {
                std::string p_icon = !icon_cfg.resolved_path.empty() ?
                                     icon_cfg.resolved_path :
                                     (fs::path(project->root_path()) / icon_cfg.raw_path).string();
                std::string icon_err;
                if (pack::NroPackBackend::is_valid_jpeg(p_icon, icon_err)) {
                    add_check(CheckItem{
                        .id = "pack.project_icon",
                        .category = "Project Assets",
                        .status = CheckStatus::Pass,
                        .message = "Application icon verified as valid JPEG (" + p_icon + ")",
                        .path = p_icon,
                        .remedy_suggestion = ""
                    });
                } else {
                    add_check(CheckItem{
                        .id = "pack.project_icon",
                        .category = "Project Assets",
                        .status = CheckStatus::Error,
                        .message = "Application icon invalid: " + icon_err,
                        .path = p_icon,
                        .remedy_suggestion = "Ensure icon file exists and is in JPEG (.jpg) format."
                    });
                }
            }

            const auto& romfs_cfg = mf.assets().romfs;
            if (romfs_cfg.enabled && !romfs_cfg.raw_path.empty()) {
                std::string p_romfs = !romfs_cfg.resolved_path.empty() ?
                                      romfs_cfg.resolved_path :
                                      (fs::path(project->root_path()) / romfs_cfg.raw_path).string();
                if (fs::exists(p_romfs) && fs::is_directory(p_romfs)) {
                    add_check(CheckItem{
                        .id = "pack.romfs",
                        .category = "Project Assets",
                        .status = CheckStatus::Pass,
                        .message = "Project RomFS asset directory found (" + p_romfs + ")",
                        .path = p_romfs,
                        .remedy_suggestion = ""
                    });
                } else {
                    add_check(CheckItem{
                        .id = "pack.romfs",
                        .category = "Project Assets",
                        .status = CheckStatus::Error,
                        .message = "Configured RomFS path does not exist or is not a directory: " + p_romfs,
                        .path = p_romfs,
                        .remedy_suggestion = "Create the RomFS directory or update the 'assets.romfs' path in nxapp.yaml."
                    });
                }
            }
        }
    }

    // 9. Deployment Tools & Target Devices
    if (profile == DoctorProfile::All || profile == DoctorProfile::Deploy || profile == DoctorProfile::Run) {
        const auto* nxlink = env.get_tool("nxlink");
        if (nxlink && nxlink->usable) {
            add_check(CheckItem{
                .id = "tool.nxlink",
                .category = "Deployment Tools",
                .status = CheckStatus::Pass,
                .message = "nxlink wireless deployment tool found",
                .path = nxlink->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.nxlink",
                .category = "Deployment Tools",
                .status = (profile == DoctorProfile::Run || profile == DoctorProfile::Deploy) ? CheckStatus::Error : CheckStatus::Warning,
                .message = "nxlink not found (required for wireless homebrew deployment/run)",
                .path = "",
                .remedy_suggestion = "Install switch-tools via 'dkp-pacman -S switch-tools' or add nxlink to PATH."
            });
        }

        // Target Devices Configuration
        std::string proj_root = (project.has_value() && project->is_valid()) ? project->root_path() : "";
        device::DeviceManager dm("", proj_root);
        auto dev_list = dm.list_devices();
        auto def_dev = dm.get_default_device();

        if (def_dev.has_value()) {
            add_check(CheckItem{
                .id = "device.config",
                .category = "Target Devices",
                .status = CheckStatus::Pass,
                .message = "Default device configured: " + def_dev->id + " (" + def_dev->host + ")",
                .path = dm.config_path(),
                .remedy_suggestion = ""
            });
        } else if (!dev_list.empty()) {
            add_check(CheckItem{
                .id = "device.config",
                .category = "Target Devices",
                .status = CheckStatus::Warning,
                .message = std::to_string(dev_list.size()) + " devices configured, but no default device set",
                .path = dm.config_path(),
                .remedy_suggestion = "Set a default device with 'nxdev devices set-default <id>'."
            });
        } else {
            add_check(CheckItem{
                .id = "device.config",
                .category = "Target Devices",
                .status = CheckStatus::Info,
                .message = "No target Switch devices configured (use 'nxdev devices add' or pass --host <ip>)",
                .path = dm.config_path(),
                .remedy_suggestion = "Register a device with 'nxdev devices add <name> --host <ip>'."
            });
        }

        // Crash Diagnostics & Symbolizer Tool
        const auto* addr2line = env.get_tool("aarch64-none-elf-addr2line");
        if (addr2line && addr2line->usable) {
            add_check(CheckItem{
                .id = "tool.addr2line",
                .category = "Crash Diagnostics",
                .status = CheckStatus::Pass,
                .message = "AArch64 Addr2line found for crash symbolization (" + addr2line->version + ")",
                .path = addr2line->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.addr2line",
                .category = "Crash Diagnostics",
                .status = CheckStatus::Info,
                .message = "aarch64-none-elf-addr2line not found (crash address symbolization will be limited)",
                .path = "",
                .remedy_suggestion = "Ensure devkitA64 toolchain is installed."
            });
        }
    }

    // 10. Build System Tools
    {
        const auto* cmake = env.get_tool("cmake");
        if (cmake && cmake->usable) {
            add_check(CheckItem{
                .id = "tool.cmake",
                .category = "Build System",
                .status = CheckStatus::Pass,
                .message = "CMake found (" + cmake->version + ")",
                .path = cmake->path,
                .remedy_suggestion = ""
            });
        } else {
            add_check(CheckItem{
                .id = "tool.cmake",
                .category = "Build System",
                .status = CheckStatus::Error,
                .message = "CMake 3.20+ is required on host system but was not found",
                .path = "",
                .remedy_suggestion = "Install CMake 3.20 or newer via your host system package manager."
            });
        }

        if (profile == DoctorProfile::All || profile == DoctorProfile::Build) {
            add_check(CheckItem{
                .id = "cmake.toolchain.switch",
                .category = "Build System",
                .status = CheckStatus::Pass,
                .message = "NXDev Switch CMake toolchain (NXDevSwitch.cmake) available",
                .path = "",
                .remedy_suggestion = ""
            });
        }
    }
}

void Doctor::print_human_report(std::ostream& os) const {
    os << "=== NXDev Doctor Diagnostic Report ===\n\n";

    std::string current_cat;
    for (const auto& c : checks_) {
        if (c.category != current_cat) {
            current_cat = c.category;
            os << "[" << current_cat << "]\n";
        }
        os << "  " << c.symbol() << " " << c.message << "\n";
        if (!c.remedy_suggestion.empty() && (c.status == CheckStatus::Warning || c.status == CheckStatus::Error)) {
            os << "      -> Remedy: " << c.remedy_suggestion << "\n";
        }
    }

    os << "\nSummary: " << pass_count_ << " passed, "
       << warning_count_ << " warning(s), "
       << error_count_ << " error(s), "
       << info_count_ << " info.\n";

    if (error_count_ == 0) {
        if (warning_count_ == 0) {
            os << "Status: Healthy! Your NXDev environment is fully operational.\n";
        } else {
            os << "Status: Ready with advisory warnings.\n";
        }
    } else {
        os << "Status: Missing required toolchain components. See remedies above.\n";
    }
}

static std::string escape_json_str(std::string_view s) {
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

std::string Doctor::to_json() const {
    std::ostringstream j;
    j << "{\n";
    j << "  \"status\": \"" << (error_count_ > 0 ? "error" : (warning_count_ > 0 ? "warning" : "pass")) << "\",\n";
    j << "  \"summary\": {\n";
    j << "    \"passed\": " << pass_count_ << ",\n";
    j << "    \"warnings\": " << warning_count_ << ",\n";
    j << "    \"errors\": " << error_count_ << ",\n";
    j << "    \"info\": " << info_count_ << "\n";
    j << "  },\n";
    j << "  \"checks\": [\n";

    for (size_t i = 0; i < checks_.size(); ++i) {
        const auto& c = checks_[i];
        j << "    {\n";
        j << "      \"id\": \"" << escape_json_str(c.id) << "\",\n";
        j << "      \"category\": \"" << escape_json_str(c.category) << "\",\n";
        j << "      \"status\": \"" << c.status_string() << "\",\n";
        j << "      \"message\": \"" << escape_json_str(c.message) << "\",\n";
        j << "      \"path\": \"" << escape_json_str(c.path) << "\",\n";
        j << "      \"remedy\": \"" << escape_json_str(c.remedy_suggestion) << "\"\n";
        j << "    }" << (i + 1 < checks_.size() ? "," : "") << "\n";
    }

    j << "  ]\n";
    j << "}\n";
    return j.str();
}

} // namespace nxdev::doctor
