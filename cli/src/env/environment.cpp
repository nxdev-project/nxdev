#include <nxdev/env/environment.hpp>
#include <nxdev/exec/process.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace nxdev::env {

namespace fs = std::filesystem;

static std::string first_line(const std::string& str) {
    auto pos = str.find_first_of("\r\n");
    if (pos == std::string::npos) return str;
    return str.substr(0, pos);
}

const ToolInfo* Environment::get_tool(const std::string& executable_name) const {
    auto it = tools_.find(executable_name);
    if (it != tools_.end()) {
        return &it->second;
    }
    return nullptr;
}

ToolInfo Environment::probe_tool(
    const std::string& name,
    const std::string& exe,
    ToolCategory cat,
    const std::vector<std::string>& candidate_dirs,
    const std::string& version_flag
) {
    ToolInfo info;
    info.name = name;
    info.executable_name = exe;
    info.category = cat;

    // 1. Search candidate directories first (devkitPro/devkitA64 priority)
    for (const auto& dir : candidate_dirs) {
        if (dir.empty()) continue;
        fs::path p = fs::path(dir) / exe;
        if (fs::exists(p)) {
            info.path = p.string();
            info.exists = true;
            info.source = ToolSource::DevkitProTree;
            break;
        }
    }

    // 2. If not found in candidate trees, search PATH
    if (!info.exists) {
        auto path_env = std::getenv("PATH");
        if (path_env) {
            std::string path_str(path_env);
            std::istringstream ss(path_str);
            std::string dir_token;
            while (std::getline(ss, dir_token, ':')) {
                if (dir_token.empty()) continue;
                fs::path p = fs::path(dir_token) / exe;
                if (fs::exists(p)) {
                    info.path = p.string();
                    info.exists = true;
                    info.source = ToolSource::SystemPath;
                    break;
                }
            }
        }
    }

    if (info.exists) {
        // Probe execution and version
        auto probe = exec::ProcessExecutor::probe_version(info.path, version_flag, 2000);
        info.is_executable = (probe.exit_code != 127);
        info.usable = probe.success || info.is_executable;
        if (!probe.stdout_output.empty()) {
            info.version = first_line(probe.stdout_output);
        } else if (!probe.stderr_output.empty()) {
            info.version = first_line(probe.stderr_output);
        } else {
            info.version = "version unavailable";
        }
    } else {
        info.source = ToolSource::NotFound;
        info.usable = false;
    }

    return info;
}

Environment Environment::detect(
    const config::HostConfig& cfg,
    const std::optional<std::string>& cli_devkitpro,
    const std::optional<std::string>& cli_devkita64
) {
    Environment env;
    env.host_ = HostInfo::detect();
    env.config_ = cfg;

    // Helper to check if dir is on PATH
    auto is_dir_on_path = [](const fs::path& dir_to_check) -> bool {
        const char* p_env = std::getenv("PATH");
        if (!p_env || dir_to_check.empty()) return false;
        std::string p_str(p_env);
        std::istringstream ss(p_str);
        std::string item;
        while (std::getline(ss, item, ':')) {
            if (!item.empty()) {
                if (item == dir_to_check.string()) return true;
                std::error_code ec;
                if (fs::exists(item, ec) && fs::exists(dir_to_check, ec) && fs::equivalent(item, dir_to_check, ec)) {
                    return true;
                }
            }
        }
        return false;
    };

    // 1. Resolve DEVKITPRO
    if (cli_devkitpro.has_value() && !cli_devkitpro->empty()) {
        env.devkitpro_.path = *cli_devkitpro;
        env.devkitpro_.source = ToolSource::CLIOverride;
    } else if (cfg.toolchain().devkitpro.has_value() && !cfg.toolchain().devkitpro->empty()) {
        env.devkitpro_.path = *cfg.toolchain().devkitpro;
        env.devkitpro_.source = ToolSource::LocalConfig;
    } else if (const char* env_dkp = std::getenv("DEVKITPRO")) {
        env.devkitpro_.path = env_dkp;
        env.devkitpro_.source = ToolSource::EnvironmentVar;
        env.devkitpro_.env_var_set = true;
    } else if (fs::exists("/opt/devkitpro")) {
        env.devkitpro_.path = "/opt/devkitpro";
        env.devkitpro_.source = ToolSource::DevkitProTree;
    }

    if (const char* env_dkp = std::getenv("DEVKITPRO")) {
        if (*env_dkp != '\0') env.devkitpro_.env_var_set = true;
    }

    if (!env.devkitpro_.path.empty()) {
        env.devkitpro_.is_configured = true;
        if (fs::exists(env.devkitpro_.path) && fs::is_directory(env.devkitpro_.path)) {
            env.devkitpro_.is_valid = true;
            env.devkitpro_.bin_in_path = is_dir_on_path(fs::path(env.devkitpro_.path) / "tools" / "bin");
        }
    }

    // 2. Resolve DEVKITA64
    if (cli_devkita64.has_value() && !cli_devkita64->empty()) {
        env.devkita64_.path = *cli_devkita64;
        env.devkita64_.source = ToolSource::CLIOverride;
    } else if (cfg.toolchain().devkita64.has_value() && !cfg.toolchain().devkita64->empty()) {
        env.devkita64_.path = *cfg.toolchain().devkita64;
        env.devkita64_.source = ToolSource::LocalConfig;
    } else if ((env.devkitpro_.source == ToolSource::CLIOverride || env.devkitpro_.source == ToolSource::LocalConfig) &&
               env.devkitpro_.is_valid && fs::exists(fs::path(env.devkitpro_.path) / "devkitA64")) {
        env.devkita64_.path = (fs::path(env.devkitpro_.path) / "devkitA64").string();
        env.devkita64_.source = ToolSource::DevkitProTree;
    } else if (const char* env_dka = std::getenv("DEVKITA64")) {
        env.devkita64_.path = env_dka;
        env.devkita64_.source = ToolSource::EnvironmentVar;
        env.devkita64_.env_var_set = true;
    } else if (env.devkitpro_.is_valid) {
        fs::path dka_candidate = fs::path(env.devkitpro_.path) / "devkitA64";
        if (fs::exists(dka_candidate)) {
            env.devkita64_.path = dka_candidate.string();
            env.devkita64_.source = ToolSource::DevkitProTree;
        }
    }

    if (const char* env_dka = std::getenv("DEVKITA64")) {
        if (*env_dka != '\0') env.devkita64_.env_var_set = true;
    }

    if (!env.devkita64_.path.empty()) {
        env.devkita64_.is_configured = true;
        if (fs::exists(env.devkita64_.path) && fs::is_directory(env.devkita64_.path)) {
            env.devkita64_.is_valid = true;
            env.devkita64_.bin_in_path = is_dir_on_path(fs::path(env.devkita64_.path) / "bin");
        }
    }

    // Check DEVKITPRO and DEVKITA64 consistency
    if (env.devkitpro_.is_valid && env.devkita64_.is_configured) {
        fs::path expected_dka = fs::path(env.devkitpro_.path) / "devkitA64";
        std::error_code ec;
        if (fs::exists(env.devkita64_.path, ec) && fs::exists(expected_dka, ec)) {
            env.devkita64_.is_consistent = fs::equivalent(env.devkita64_.path, expected_dka, ec) ||
                                          (fs::path(env.devkita64_.path).lexically_normal() == expected_dka.lexically_normal());
        } else {
            env.devkita64_.is_consistent = (fs::path(env.devkita64_.path).lexically_normal() == expected_dka.lexically_normal());
        }
    }

    // 3. Resolve libnx
    if (env.devkitpro_.is_valid) {
        fs::path libnx_inc = fs::path(env.devkitpro_.path) / "libnx" / "include" / "switch.h";
        fs::path libnx_alt = fs::path(env.devkitpro_.path) / "libnx" / "include" / "libnx.h";
        fs::path libnx_lib = fs::path(env.devkitpro_.path) / "libnx" / "lib" / "libnx.a";

        if ((fs::exists(libnx_inc) || fs::exists(libnx_alt)) && fs::exists(libnx_lib)) {
            env.libnx_.found = true;
            env.libnx_.include_path = (fs::path(env.devkitpro_.path) / "libnx" / "include").string();
            env.libnx_.library_path = libnx_lib.string();
            env.libnx_.version = "installed (version unknown)";

            fs::path header_to_check = fs::exists(libnx_inc) ? libnx_inc : libnx_alt;
            std::ifstream hfile(header_to_check);
            std::string line;
            while (std::getline(hfile, line)) {
                if (line.find("LIBNX_VERSION") != std::string::npos) {
                    auto quote_start = line.find('\"');
                    auto quote_end = line.rfind('\"');
                    if (quote_start != std::string::npos && quote_end != std::string::npos && quote_end > quote_start) {
                        env.libnx_.version = line.substr(quote_start + 1, quote_end - quote_start - 1);
                        break;
                    }
                }
            }
        }
    }

    // 4. Resolve switch-tools
    if (env.devkitpro_.is_valid) {
        fs::path tools_bin = fs::path(env.devkitpro_.path) / "tools" / "bin";
        if (fs::exists(tools_bin) && fs::is_directory(tools_bin)) {
            env.switch_tools_.is_found = true;
            env.switch_tools_.path = tools_bin.string();
            env.switch_tools_.source = ToolSource::DevkitProTree;
        }
    }

    // 5. Resolve NXDevSDK
    // Precedence: explicit config override > NXDEV_SDK_ROOT > standard platform path > source-tree fallback
    auto cfg_sdk = cfg.get_value("sdk.root");
    if (!cfg_sdk) cfg_sdk = cfg.get_value("toolchain.sdk_root");

    const char* env_sdk = std::getenv("NXDEV_SDK_ROOT");

    if (cfg_sdk && !cfg_sdk->empty() && fs::exists(*cfg_sdk)) {
        env.sdk_.path = *cfg_sdk;
        env.sdk_.source = SdkSource::LocalConfig;
        env.sdk_.found = true;
    } else if (env_sdk && *env_sdk && fs::exists(env_sdk)) {
        env.sdk_.path = env_sdk;
        env.sdk_.source = SdkSource::EnvironmentVar;
        env.sdk_.found = true;
        env.sdk_.env_root_set = true;
    } else if (fs::exists("/opt/nxdev") && (fs::exists("/opt/nxdev/include/nxdev/nxdev.hpp") || fs::exists("/opt/nxdev/share/nxdev/sdk.json"))) {
        env.sdk_.path = "/opt/nxdev";
        env.sdk_.source = SdkSource::StandardInstallPath;
        env.sdk_.found = true;
        env.sdk_.is_installed = true;
    } else if (fs::exists("C:/NXDevSDK") && (fs::exists("C:/NXDevSDK/include/nxdev/nxdev.hpp") || fs::exists("C:/NXDevSDK/share/nxdev/sdk.json"))) {
        env.sdk_.path = "C:/NXDevSDK";
        env.sdk_.source = SdkSource::StandardInstallPath;
        env.sdk_.found = true;
        env.sdk_.is_installed = true;
    } else {
        // Search parent directories for repository source-tree SDK
        fs::path p = fs::current_path();
        for (int i = 0; i < 4 && p != p.root_path(); ++i) {
            if (fs::exists(p / "sdk" / "core" / "include" / "nxdev" / "nxdev.hpp")) {
                env.sdk_.path = (p / "sdk").string();
                env.sdk_.source = SdkSource::SourceTree;
                env.sdk_.found = true;
                env.sdk_.is_installed = false;
                break;
            }
            p = p.parent_path();
        }
    }

    if (env.sdk_.found) {
        fs::path sdk_json_path = fs::path(env.sdk_.path) / "share" / "nxdev" / "sdk.json";
        if (fs::exists(sdk_json_path)) {
            env.sdk_.is_installed = true;
            std::ifstream jf(sdk_json_path);
            std::string content((std::istreambuf_iterator<char>(jf)), std::istreambuf_iterator<char>());
            auto vpos = content.find("\"version\"");
            if (vpos != std::string::npos) {
                auto q1 = content.find('\"', vpos + 9);
                auto q2 = (q1 != std::string::npos) ? content.find('\"', q1 + 1) : std::string::npos;
                if (q1 != std::string::npos && q2 != std::string::npos) {
                    env.sdk_.version = content.substr(q1 + 1, q2 - q1 - 1);
                }
            }
        } else {
#ifdef NXDEV_VERSION
            env.sdk_.version = NXDEV_VERSION;
#else
            env.sdk_.version = "0.1.0-beta.1";
#endif
        }
        env.sdk_.is_valid = true;

        // Check if $NXDEV_SDK_ROOT/bin is on PATH
        fs::path bin_dir = fs::path(env.sdk_.path) / "bin";
        const char* path_env = std::getenv("PATH");
        if (path_env) {
            std::string p_str(path_env);
            std::istringstream ss(p_str);
            std::string item;
            while (std::getline(ss, item, ':')) {
                if (!item.empty()) {
                    if (item == bin_dir.string()) {
                        env.sdk_.bin_in_path = true;
                        break;
                    }
                    std::error_code ec;
                    if (fs::exists(item, ec) && fs::exists(bin_dir, ec) && fs::equivalent(item, bin_dir, ec)) {
                        env.sdk_.bin_in_path = true;
                        break;
                    }
                }
            }
        }

        fs::path cm1 = fs::path(env.sdk_.path) / "share" / "nxdev" / "cmake" / "NXDevConfig.cmake";
        fs::path cm2 = fs::path(env.sdk_.path) / ".." / "cmake" / "NXDevConfig.cmake";
        env.sdk_.has_cmake_package = (fs::exists(cm1) || fs::exists(cm2));
    }

    // Prepare candidate search paths for tools
    std::vector<std::string> compiler_search_dirs;
    if (env.devkita64_.is_valid) {
        compiler_search_dirs.push_back((fs::path(env.devkita64_.path) / "bin").string());
    }

    std::vector<std::string> switch_tool_dirs;
    if (env.switch_tools_.is_found) {
        switch_tool_dirs.push_back(env.switch_tools_.path);
    }

    // 6. Probe core compiler tools
    env.tools_["aarch64-none-elf-gcc"] = probe_tool("AArch64 GCC", "aarch64-none-elf-gcc", ToolCategory::CoreCompiler, compiler_search_dirs);
    env.tools_["aarch64-none-elf-g++"] = probe_tool("AArch64 G++", "aarch64-none-elf-g++", ToolCategory::CoreCompiler, compiler_search_dirs);
    env.tools_["aarch64-none-elf-ar"] = probe_tool("AArch64 AR", "aarch64-none-elf-ar", ToolCategory::CoreCompiler, compiler_search_dirs);
    env.tools_["aarch64-none-elf-objcopy"] = probe_tool("AArch64 Objcopy", "aarch64-none-elf-objcopy", ToolCategory::CoreCompiler, compiler_search_dirs);
    env.tools_["aarch64-none-elf-addr2line"] = probe_tool("AArch64 Addr2line", "aarch64-none-elf-addr2line", ToolCategory::CoreCompiler, compiler_search_dirs, "--version");

    // 7. Probe packaging & deployment tools
    env.tools_["elf2nro"] = probe_tool("elf2nro", "elf2nro", ToolCategory::Packaging, switch_tool_dirs, "--version");
    env.tools_["elf2nso"] = probe_tool("elf2nso", "elf2nso", ToolCategory::Packaging, switch_tool_dirs);
    env.tools_["npdmtool"] = probe_tool("npdmtool", "npdmtool", ToolCategory::Packaging, switch_tool_dirs);
    env.tools_["nacptool"] = probe_tool("nacptool", "nacptool", ToolCategory::Packaging, switch_tool_dirs, "--version");
    env.tools_["hacbrewpack"] = probe_tool("hacBrewPack", "hacbrewpack", ToolCategory::Packaging, switch_tool_dirs, "--version");
    env.tools_["nxlink"] = probe_tool("nxlink", "nxlink", ToolCategory::Deployment, switch_tool_dirs, "-h");

    // 8. Probe build system & VCS tools
    env.tools_["cmake"] = probe_tool("CMake", "cmake", ToolCategory::BuildSystem, {}, "--version");
    env.tools_["ninja"] = probe_tool("Ninja", "ninja", ToolCategory::BuildSystem, {}, "--version");
    env.tools_["make"] = probe_tool("Make", "make", ToolCategory::BuildSystem, {}, "--version");
    env.tools_["git"] = probe_tool("Git", "git", ToolCategory::VCS, {}, "--version");

    return env;
}

std::string Environment::summary(const std::optional<project::NXDevProject>& current_project) const {
    std::ostringstream oss;
    oss << "Host\n";
    oss << "  OS:           " << host_.os_name() << "\n";
    oss << "  Architecture: " << host_.arch_name() << "\n";
    if (host_.is_wsl) {
        oss << "  WSL:          " << host_.wsl_version_name() << " (" << host_.wsl_distro_name << ")\n";
    }

    oss << "\nNXDev\n";
#ifdef NXDEV_VERSION
    oss << "  Version:      " << NXDEV_VERSION << "\n";
#else
    oss << "  Version:      0.1.0-dev\n";
#endif
    if (current_project.has_value()) {
        oss << "  Project:      " << current_project->root_path() << "\n";
        oss << "  Manifest:     " << current_project->manifest_path() << "\n";
    } else {
        oss << "  Project:      (No active project found)\n";
    }

    oss << "\nNXDevSDK\n";
    oss << "  SDK Root:     " << (sdk_.found ? sdk_.path : "Not Found") << " [" << sdk_.source_name() << "]\n";
    oss << "  SDK Version:  " << (sdk_.found ? sdk_.version : "N/A") << "\n";
    oss << "  Env Set:      " << (sdk_.env_root_set ? "Yes (NXDEV_SDK_ROOT)" : "No (using default / discovery)") << "\n";
    oss << "  bin on PATH:  " << (sdk_.bin_in_path ? "Yes" : "No") << "\n";
    oss << "  CMake Pkg:    " << (sdk_.has_cmake_package ? "Available" : "Not Found") << "\n";

    oss << "\ndevkitPro Toolchain\n";
    oss << "  DEVKITPRO:    " << (devkitpro_.is_configured ? devkitpro_.path : "Not Configured") << " [" << devkitpro_.source_name() << "]\n";
    oss << "  devkitA64:    " << (devkita64_.is_configured ? devkita64_.path : "Not Configured") << " [" << devkita64_.source_name() << "]\n";
    oss << "  libnx:        " << (libnx_.found ? "Installed" : "Not Found") << "\n";
    oss << "  switch-tools: " << (switch_tools_.is_found ? switch_tools_.path : "Not Found") << "\n";

    oss << "\nTool Status\n";
    for (const auto& [exe, t] : tools_) {
        oss << "  " << exe << ": " << (t.usable ? ("Found (" + t.path + ")") : "Not Found") << "\n";
    }

    return oss.str();
}

std::string Environment::to_json(const std::optional<project::NXDevProject>& current_project) const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"host\": {\n";
    oss << "    \"os\": \"" << host_.os_name() << "\",\n";
    oss << "    \"arch\": \"" << host_.arch_name() << "\",\n";
    oss << "    \"isWSL\": " << (host_.is_wsl ? "true" : "false") << ",\n";
    oss << "    \"wslVersion\": \"" << host_.wsl_version_name() << "\",\n";
    oss << "    \"wslDistro\": \"" << host_.wsl_distro_name << "\"\n";
    oss << "  },\n";

    oss << "  \"sdk\": {\n";
    oss << "    \"found\": " << (sdk_.found ? "true" : "false") << ",\n";
    oss << "    \"isInstalled\": " << (sdk_.is_installed ? "true" : "false") << ",\n";
    oss << "    \"isValid\": " << (sdk_.is_valid ? "true" : "false") << ",\n";
    oss << "    \"envRootSet\": " << (sdk_.env_root_set ? "true" : "false") << ",\n";
    oss << "    \"binInPath\": " << (sdk_.bin_in_path ? "true" : "false") << ",\n";
    oss << "    \"hasCmakePackage\": " << (sdk_.has_cmake_package ? "true" : "false") << ",\n";
    oss << "    \"path\": \"" << sdk_.path << "\",\n";
    oss << "    \"version\": \"" << sdk_.version << "\",\n";
    oss << "    \"source\": \"" << sdk_.source_name() << "\"\n";
    oss << "  },\n";

    oss << "  \"toolchain\": {\n";
    oss << "    \"devkitpro\": {\n";
    oss << "      \"isConfigured\": " << (devkitpro_.is_configured ? "true" : "false") << ",\n";
    oss << "      \"isValid\": " << (devkitpro_.is_valid ? "true" : "false") << ",\n";
    oss << "      \"path\": \"" << devkitpro_.path << "\",\n";
    oss << "      \"source\": \"" << devkitpro_.source_name() << "\"\n";
    oss << "    },\n";
    oss << "    \"devkita64\": {\n";
    oss << "      \"isConfigured\": " << (devkita64_.is_configured ? "true" : "false") << ",\n";
    oss << "      \"isValid\": " << (devkita64_.is_valid ? "true" : "false") << ",\n";
    oss << "      \"path\": \"" << devkita64_.path << "\",\n";
    oss << "      \"source\": \"" << devkita64_.source_name() << "\"\n";
    oss << "    },\n";
    oss << "    \"libnx\": {\n";
    oss << "      \"found\": " << (libnx_.found ? "true" : "false") << ",\n";
    oss << "      \"includePath\": \"" << libnx_.include_path << "\",\n";
    oss << "      \"libraryPath\": \"" << libnx_.library_path << "\",\n";
    oss << "      \"version\": \"" << libnx_.version << "\"\n";
    oss << "    },\n";
    oss << "    \"switchTools\": {\n";
    oss << "      \"isFound\": " << (switch_tools_.is_found ? "true" : "false") << ",\n";
    oss << "      \"path\": \"" << switch_tools_.path << "\"\n";
    oss << "    }\n";
    oss << "  },\n";

    oss << "  \"project\": " << (current_project.has_value() ? current_project->info_json() : "null") << ",\n";

    oss << "  \"tools\": {\n";
    size_t tool_idx = 0;
    for (const auto& [exe, t] : tools_) {
        oss << "    \"" << exe << "\": {\n";
        oss << "      \"name\": \"" << t.name << "\",\n";
        oss << "      \"category\": \"" << t.category_name() << "\",\n";
        oss << "      \"source\": \"" << t.source_name() << "\",\n";
        oss << "      \"path\": \"" << t.path << "\",\n";
        oss << "      \"version\": \"" << t.version << "\",\n";
        oss << "      \"usable\": " << (t.usable ? "true" : "false") << "\n";
        oss << "    }" << (tool_idx + 1 < tools_.size() ? "," : "") << "\n";
        tool_idx++;
    }
    oss << "  }\n";
    oss << "}\n";

    return oss.str();
}

} // namespace nxdev::env
