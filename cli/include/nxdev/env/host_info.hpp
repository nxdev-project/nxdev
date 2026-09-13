#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <map>

namespace nxdev::env {

enum class OperatingSystem {
    Linux,
    Windows,
    MacOS,
    Unknown
};

enum class Architecture {
    X86_64,
    AArch64,
    ARM,
    X86,
    Unknown
};

enum class WslVersion {
    None,
    Wsl1,
    Wsl2
};

struct HostInfo {
    OperatingSystem os{OperatingSystem::Unknown};
    Architecture arch{Architecture::Unknown};
    bool is_wsl{false};
    WslVersion wsl_version{WslVersion::None};
    std::string wsl_distro_name;
    bool has_wsl_interop{false};
    bool has_wslpath{false};
    std::string shell_name;
    std::string kernel_release;

    [[nodiscard]] std::string os_name() const;
    [[nodiscard]] std::string arch_name() const;
    [[nodiscard]] std::string wsl_version_name() const;
    [[nodiscard]] std::string summary() const;

    [[nodiscard]] bool is_mounted_windows_path(const std::string& path) const noexcept;
    [[nodiscard]] std::string to_windows_path(const std::string& wsl_path) const;
    [[nodiscard]] std::string to_wsl_path(const std::string& windows_path) const;

    /**
     * @brief Performs live detection of the current host environment.
     */
    static HostInfo detect();

    /**
     * @brief Mockable detector for unit testing.
     */
    static HostInfo detect_mock(
        OperatingSystem os,
        Architecture arch,
        const std::string& proc_version,
        const std::string& osrelease,
        const std::map<std::string, std::string>& env_vars
    );
};

} // namespace nxdev::env
