#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <functional>

namespace nxdev::pack {

struct ProcessOptions {
    uint32_t timeout_ms{60000};
    std::string working_dir;
    std::string log_file_path;
    std::string stdout_log_path;
    std::string stderr_log_path;
    size_t max_memory_bytes{0};          // 0 = no hard ceiling enforced by executor
    size_t max_output_tail_bytes{65536}; // 64 KiB sliding in-memory tail
    std::function<void(std::string_view stream, std::string_view chunk)> output_callback;
};

struct ProcessResult {
    int exit_code{-1};
    int termination_signal{0};
    bool terminated_by_signal{false};
    bool timed_out{false};
    bool memory_limit_exceeded{false};
    bool possible_oom_killed{false};
    size_t peak_rss_bytes{0};
    uint64_t duration_ms{0};
    std::string stdout_output; // Bounded diagnostic tail
    std::string stderr_output; // Bounded diagnostic tail
    std::string log_path;
    bool success{false};

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
     * @brief Executes a process using direct argument vector execution with full monitoring options.
     */
    [[nodiscard]] static ProcessResult execute(
        const std::string& binary,
        const std::vector<std::string>& args,
        const ProcessOptions& options
    );

    /**
     * @brief Convenience overload for backward compatibility.
     */
    [[nodiscard]] static ProcessResult execute(
        const std::string& binary,
        const std::vector<std::string>& args,
        uint32_t timeout_ms = 60000,
        const std::string& working_dir = ""
    );

    /**
     * @brief Inspects current Linux available memory in bytes from /proc/meminfo.
     */
    [[nodiscard]] static size_t get_available_memory_bytes();

    /**
     * @brief Inspects process RSS in bytes from /proc/<pid>/statm.
     */
    [[nodiscard]] static size_t get_process_rss_bytes(int pid);

    /**
     * @brief Checks if running inside Windows Subsystem for Linux (WSL).
     */
    [[nodiscard]] static bool is_wsl_environment();
};

} // namespace nxdev::pack
