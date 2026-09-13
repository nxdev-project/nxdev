#pragma once

#include <nxdev/env/host_info.hpp>
#include <nxdev/env/toolchain_info.hpp>
#include <nxdev/config/host_config.hpp>
#include <nxdev/project/project.hpp>
#include <map>
#include <memory>
#include <vector>

namespace nxdev::env {

class Environment {
public:
    Environment() = default;
    ~Environment() = default;

    [[nodiscard]] const HostInfo& host() const noexcept { return host_; }
    [[nodiscard]] const config::HostConfig& config() const noexcept { return config_; }
    [[nodiscard]] const DevkitProInfo& devkitpro() const noexcept { return devkitpro_; }
    [[nodiscard]] const DevkitA64Info& devkita64() const noexcept { return devkita64_; }
    [[nodiscard]] const LibnxInfo& libnx() const noexcept { return libnx_; }
    [[nodiscard]] const SwitchToolsInfo& switch_tools() const noexcept { return switch_tools_; }
    [[nodiscard]] const SdkInfo& sdk() const noexcept { return sdk_; }
    [[nodiscard]] const std::map<std::string, ToolInfo>& tools() const noexcept { return tools_; }

    [[nodiscard]] const ToolInfo* get_tool(const std::string& executable_name) const;

    [[nodiscard]] std::string summary(const std::optional<project::NXDevProject>& current_project = std::nullopt) const;
    [[nodiscard]] std::string to_json(const std::optional<project::NXDevProject>& current_project = std::nullopt) const;

    /**
     * @brief Performs comprehensive environment discovery.
     */
    static Environment detect(
        const config::HostConfig& cfg,
        const std::optional<std::string>& cli_devkitpro = std::nullopt,
        const std::optional<std::string>& cli_devkita64 = std::nullopt
    );

private:
    HostInfo host_;
    config::HostConfig config_;
    DevkitProInfo devkitpro_;
    DevkitA64Info devkita64_;
    LibnxInfo libnx_;
    SwitchToolsInfo switch_tools_;
    SdkInfo sdk_;
    std::map<std::string, ToolInfo> tools_;

    static ToolInfo probe_tool(
        const std::string& name,
        const std::string& exe,
        ToolCategory cat,
        const std::vector<std::string>& candidate_dirs,
        const std::string& version_flag = "--version"
    );
};

} // namespace nxdev::env
