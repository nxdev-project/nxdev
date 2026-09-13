#include <nxdev/env/toolchain_info.hpp>

namespace nxdev::env {

std::string tool_source_to_string(ToolSource source) {
    switch (source) {
        case ToolSource::CLIOverride: return "CLI Override";
        case ToolSource::LocalConfig: return "Local Config";
        case ToolSource::GlobalConfig: return "Global Config";
        case ToolSource::EnvironmentVar: return "Environment Variable";
        case ToolSource::DevkitProTree: return "devkitPro Tree";
        case ToolSource::SystemPath: return "PATH";
        case ToolSource::NotFound: return "Not Found";
    }
    return "Not Found";
}

std::string ToolInfo::category_name() const {
    switch (category) {
        case ToolCategory::CoreCompiler: return "Core Compiler";
        case ToolCategory::Packaging: return "Packaging";
        case ToolCategory::Deployment: return "Deployment";
        case ToolCategory::BuildSystem: return "Build System";
        case ToolCategory::VCS: return "Version Control";
    }
    return "Other";
}

std::string sdk_source_to_string(SdkSource source) {
    switch (source) {
        case SdkSource::EnvironmentVar: return "Environment Variable (NXDEV_SDK_ROOT)";
        case SdkSource::LocalConfig: return "Local Config";
        case SdkSource::StandardInstallPath: return "Standard Path (/opt/nxdev)";
        case SdkSource::SourceTree: return "Repository Source Tree";
        case SdkSource::NotFound: return "Not Found";
    }
    return "Not Found";
}

} // namespace nxdev::env
