#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <string_view>

namespace nxdev::env {

enum class ToolCategory {
    CoreCompiler,
    Packaging,
    Deployment,
    BuildSystem,
    VCS
};

enum class ToolSource {
    CLIOverride,
    LocalConfig,
    GlobalConfig,
    EnvironmentVar,
    DevkitProTree,
    SystemPath,
    NotFound
};

std::string tool_source_to_string(ToolSource source);

struct ToolInfo {
    std::string name;
    std::string executable_name;
    ToolCategory category{ToolCategory::CoreCompiler};
    ToolSource source{ToolSource::NotFound};
    std::string path;
    std::string version;
    bool exists{false};
    bool is_executable{false};
    bool usable{false};

    [[nodiscard]] std::string category_name() const;
    [[nodiscard]] std::string source_name() const { return tool_source_to_string(source); }
};

struct DevkitProInfo {
    bool is_configured{false};
    bool is_valid{false};
    bool env_var_set{false};
    bool bin_in_path{false};
    std::string path;
    ToolSource source{ToolSource::NotFound};

    [[nodiscard]] std::string source_name() const { return tool_source_to_string(source); }
};

struct DevkitA64Info {
    bool is_configured{false};
    bool is_valid{false};
    bool env_var_set{false};
    bool is_consistent{true};
    bool bin_in_path{false};
    std::string path;
    ToolSource source{ToolSource::NotFound};

    [[nodiscard]] std::string source_name() const { return tool_source_to_string(source); }
};

struct LibnxInfo {
    bool found{false};
    std::string include_path;
    std::string library_path;
    std::string version{"installed"};
};

struct SwitchToolsInfo {
    bool is_found{false};
    std::string path;
    ToolSource source{ToolSource::NotFound};

    [[nodiscard]] std::string source_name() const { return tool_source_to_string(source); }
};

enum class SdkSource {
    EnvironmentVar,
    LocalConfig,
    StandardInstallPath,
    SourceTree,
    NotFound
};

std::string sdk_source_to_string(SdkSource source);

struct SdkInfo {
    bool found{false};
    bool is_installed{false};
    bool is_valid{false};
    bool env_root_set{false};
    bool bin_in_path{false};
    bool has_cmake_package{false};
    std::string path;
    std::string version{"0.1.0-beta.1"};
    int layout_version{1};
    SdkSource source{SdkSource::NotFound};

    [[nodiscard]] std::string source_name() const { return sdk_source_to_string(source); }
};

} // namespace nxdev::env
