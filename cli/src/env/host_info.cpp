#include <nxdev/env/host_info.hpp>
#include <nxdev/exec/process.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

namespace nxdev::env {

namespace fs = std::filesystem;

std::string HostInfo::os_name() const {
    switch (os) {
        case OperatingSystem::Linux: return is_wsl ? ("WSL (" + wsl_version_name() + ")") : "Linux";
        case OperatingSystem::Windows: return "Windows";
        case OperatingSystem::MacOS: return "macOS";
        case OperatingSystem::Unknown: return "Unknown";
    }
    return "Unknown";
}

std::string HostInfo::arch_name() const {
    switch (arch) {
        case Architecture::X86_64: return "x86_64";
        case Architecture::AArch64: return "aarch64";
        case Architecture::ARM: return "arm";
        case Architecture::X86: return "x86";
        case Architecture::Unknown: return "unknown";
    }
    return "unknown";
}

std::string HostInfo::wsl_version_name() const {
    switch (wsl_version) {
        case WslVersion::Wsl1: return "WSL1";
        case WslVersion::Wsl2: return "WSL2";
        case WslVersion::None: return "None";
    }
    return "None";
}

std::string HostInfo::summary() const {
    std::string s = os_name() + " / " + arch_name();
    if (is_wsl && !wsl_distro_name.empty()) {
        s += " (" + wsl_distro_name + ")";
    }
    return s;
}

bool HostInfo::is_mounted_windows_path(const std::string& path) const noexcept {
    if (!is_wsl) return false;
    // Check if path starts with /mnt/[a-zA-Z]/ or /mnt/[a-zA-Z]$
    if (path.rfind("/mnt/", 0) == 0 && path.size() >= 6) {
        char drive = path[5];
        if ((drive >= 'a' && drive <= 'z') || (drive >= 'A' && drive <= 'Z')) {
            if (path.size() == 6 || path[6] == '/') {
                return true;
            }
        }
    }
    return false;
}

std::string HostInfo::to_windows_path(const std::string& wsl_path) const {
    if (!is_wsl || !has_wslpath) {
        return wsl_path;
    }
    auto res = exec::ProcessExecutor::execute("wslpath", {"-w", wsl_path}, 2000);
    if (res.success && !res.stdout_output.empty()) {
        std::string out = res.stdout_output;
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
        return out;
    }
    return wsl_path;
}

std::string HostInfo::to_wsl_path(const std::string& windows_path) const {
    if (!is_wsl || !has_wslpath) {
        return windows_path;
    }
    auto res = exec::ProcessExecutor::execute("wslpath", {"-u", windows_path}, 2000);
    if (res.success && !res.stdout_output.empty()) {
        std::string out = res.stdout_output;
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
        return out;
    }
    return windows_path;
}

static std::string read_file_string(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

HostInfo HostInfo::detect() {
    HostInfo info;

    // Detect Host OS
#if defined(_WIN32)
    info.os = OperatingSystem::Windows;
#elif defined(__APPLE__)
    info.os = OperatingSystem::MacOS;
#elif defined(__linux__)
    info.os = OperatingSystem::Linux;
#else
    info.os = OperatingSystem::Unknown;
#endif

    // Detect Host Arch
#if defined(__x86_64__) || defined(_M_X64)
    info.arch = Architecture::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.arch = Architecture::AArch64;
#elif defined(__arm__) || defined(_M_ARM)
    info.arch = Architecture::ARM;
#elif defined(__i386__) || defined(_M_IX86)
    info.arch = Architecture::X86;
#else
    info.arch = Architecture::Unknown;
#endif

    // Collect environment variables & proc files
    std::map<std::string, std::string> envs;
    if (const char* d = std::getenv("WSL_DISTRO_NAME")) envs["WSL_DISTRO_NAME"] = d;
    if (const char* i = std::getenv("WSL_INTEROP")) envs["WSL_INTEROP"] = i;
    else if (fs::exists("/proc/sys/fs/binfmt_misc/WSLInterop")) envs["WSL_INTEROP"] = "enabled";
    if (const char* s = std::getenv("SHELL")) envs["SHELL"] = s;

    std::string proc_ver = read_file_string("/proc/version");
    std::string osrelease = read_file_string("/proc/sys/kernel/osrelease");

    return detect_mock(info.os, info.arch, proc_ver, osrelease, envs);
}

HostInfo HostInfo::detect_mock(
    OperatingSystem os,
    Architecture arch,
    const std::string& proc_version,
    const std::string& osrelease,
    const std::map<std::string, std::string>& env_vars
) {
    HostInfo info;
    info.os = os;
    info.arch = arch;
    info.kernel_release = osrelease;
    while (!info.kernel_release.empty() && (info.kernel_release.back() == '\n' || info.kernel_release.back() == '\r')) {
        info.kernel_release.pop_back();
    }

    auto shell_it = env_vars.find("SHELL");
    if (shell_it != env_vars.end()) {
        info.shell_name = shell_it->second;
    }

    if (os == OperatingSystem::Linux) {
        // Inspect WSL markers
        std::string proc_lower = proc_version;
        std::transform(proc_lower.begin(), proc_lower.end(), proc_lower.begin(), [](unsigned char c) { return std::tolower(c); });

        std::string osrel_lower = osrelease;
        std::transform(osrel_lower.begin(), osrel_lower.end(), osrel_lower.begin(), [](unsigned char c) { return std::tolower(c); });

        bool has_wsl_distro = env_vars.find("WSL_DISTRO_NAME") != env_vars.end();
        bool has_wsl_interop = env_vars.find("WSL_INTEROP") != env_vars.end();
        bool is_ms_kernel = proc_lower.find("microsoft") != std::string::npos || osrel_lower.find("microsoft") != std::string::npos;
        bool is_wsl_kernel = proc_lower.find("wsl") != std::string::npos || osrel_lower.find("wsl") != std::string::npos;

        if (has_wsl_distro || has_wsl_interop || is_ms_kernel || is_wsl_kernel) {
            info.is_wsl = true;
            if (has_wsl_distro) {
                info.wsl_distro_name = env_vars.at("WSL_DISTRO_NAME");
            }
            info.has_wsl_interop = has_wsl_interop;

            // Distinguish WSL1 vs WSL2: WSL2 typically has 'WSL2' or 'microsoft-standard-WSL2' in osrelease
            if (osrel_lower.find("wsl2") != std::string::npos || osrel_lower.find("microsoft-standard") != std::string::npos) {
                info.wsl_version = WslVersion::Wsl2;
            } else if (is_ms_kernel || is_wsl_kernel) {
                info.wsl_version = WslVersion::Wsl1;
            } else {
                info.wsl_version = WslVersion::Wsl2; // Default modern WSL
            }

            // Check if wslpath is available
            info.has_wslpath = (env_vars.find("HAS_WSLPATH") != env_vars.end()) || has_wsl_distro;
        }
    }

    return info;
}

} // namespace nxdev::env
