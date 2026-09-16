#pragma once

#include <nxdev/project/project.hpp>
#include <nxdev/env/environment.hpp>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace nxdev::build {

enum class BuildProfile {
    Debug,
    Release
};

std::string build_profile_to_string(BuildProfile profile);
std::optional<BuildProfile> parse_build_profile(const std::string& name);

struct BuildOptions {
    BuildProfile profile{BuildProfile::Debug};
    bool clean_first{false};
    bool fresh_configure{false};
    bool verbose{false};
    std::optional<uint32_t> parallel_jobs;
    std::optional<std::string> generator;
    std::vector<std::string> extra_cmake_args;
    size_t max_memory_bytes{0};
    bool unsafe_no_limits{false};
};

struct BuildResult {
    bool success{false};
    int exit_code{0};
    std::string profile{"debug"};
    std::string project_root;
    std::string build_directory;
    std::string target_name;
    std::string elf_path;
    std::string compile_commands_path;
    std::string stdout_output;
    std::string stderr_output;
    std::vector<std::string> diagnostics;

    // Resource diagnostics
    size_t peak_memory_bytes{0};
    uint64_t duration_ms{0};
    uint32_t parallel_jobs{1};
    std::string resource_controller;
    bool resource_limit_exceeded{false};
    std::string resource_limit_reason;

    [[nodiscard]] std::string to_json() const;
};

class BuildOrchestrator {
public:
    BuildOrchestrator() = default;
    ~BuildOrchestrator() = default;

    /**
     * @brief Computes canonical build directory: <root>/.nxdev/build/<profile>
     */
    [[nodiscard]] static std::string resolve_build_directory(
        const std::string& project_root,
        const std::string& profile_name
    );

    /**
     * @brief Assembles CMake configuration command-line arguments.
     */
    [[nodiscard]] static std::vector<std::string> generate_configure_args(
        const project::NXDevProject& project,
        const env::Environment& env,
        const BuildOptions& options,
        const std::string& build_dir
    );

    /**
     * @brief Safely removes NXDev-managed build artifacts for a project.
     */
    static bool safe_clean(const project::NXDevProject& project, std::string& error_message);

    /**
     * @brief Runs CMake configuration for the project.
     */
    static BuildResult configure(
        const project::NXDevProject& project,
        const env::Environment& env,
        const BuildOptions& options
    );

    /**
     * @brief Runs CMake configuration (if required) and builds the Switch ELF target under resource controls.
     */
    static BuildResult build(
        const project::NXDevProject& project,
        const env::Environment& env,
        const BuildOptions& options
    );
};

} // namespace nxdev::build
