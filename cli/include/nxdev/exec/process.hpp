#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nxdev::exec {

struct ProcessResult {
    int exit_code{-1};
    std::string stdout_output;
    std::string stderr_output;
    bool success{false};
    bool timed_out{false};

    [[nodiscard]] std::string combined_output() const {
        if (stdout_output.empty()) return stderr_output;
        if (stderr_output.empty()) return stdout_output;
        return stdout_output + "\n" + stderr_output;
    }
};

class ProcessExecutor {
public:
    ProcessExecutor() = default;
    ~ProcessExecutor() = default;

    /**
     * @brief Executes a process using direct argument vector execution (no shell string expansion).
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
