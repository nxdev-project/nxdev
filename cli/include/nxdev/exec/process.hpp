#pragma once

#include <nxdev/exec/resource_policy.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <nxdev/exec/controlled_runner.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace nxdev::exec {

class ProcessExecutor {
public:
    ProcessExecutor() = default;
    ~ProcessExecutor() = default;

    /**
     * @brief Executes a process with an explicit resource policy.
     */
    [[nodiscard]] static ProcessResult execute(
        const std::string& binary,
        const std::vector<std::string>& args,
        const ProcessResourcePolicy& policy
    );

    /**
     * @brief Executes a process using direct argument vector execution.
     * @param binary Path or executable name.
     * @param args Command line arguments vector.
     * @param timeout_ms Maximum time to wait in milliseconds.
     * @param working_dir Optional working directory.
     */
    [[nodiscard]] static ProcessResult execute(
        const std::string& binary,
        const std::vector<std::string>& args,
        uint32_t timeout_ms = 5000,
        const std::string& working_dir = ""
    );

    /**
     * @brief Probes a binary with non-destructive version flags (e.g. --version or -v).
     */
    [[nodiscard]] static ProcessResult probe_version(
        const std::string& binary_path,
        const std::string& version_flag = "--version",
        uint32_t timeout_ms = 3000
    );
};

} // namespace nxdev::exec
