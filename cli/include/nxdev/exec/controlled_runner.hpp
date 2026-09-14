#pragma once

#include <nxdev/exec/resource_policy.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace nxdev::exec {

struct ProcessResult {
    int exit_code{-1};
    int termination_signal{0};
    bool terminated_by_signal{false};
    bool timed_out{false};
    bool resource_limit_exceeded{false};
    ResourceLimitReason resource_limit_reason{ResourceLimitReason::None};
    bool possible_oom_killed{false};
    size_t peak_rss_bytes{0};
    uint32_t peak_process_count{1};
    size_t lowest_host_available_bytes{0};
    uint64_t duration_ms{0};
    ControllerType controller{ControllerType::None};
    std::string stdout_output; // Bounded in-memory sliding tail
    std::string stderr_output; // Bounded in-memory sliding tail
    std::string log_path;
    bool success{false};

    [[nodiscard]] std::string combined_output() const {
        if (stdout_output.empty()) return stderr_output;
        if (stderr_output.empty()) return stdout_output;
        return stdout_output + "\n" + stderr_output;
    }
};

class ControlledProcessRunner {
public:
    ControlledProcessRunner() = default;
    ~ControlledProcessRunner() = default;

    /**
     * @brief Executes a command line with active resource monitoring, process-tree tracking,
     * cgroup v2 controls (if available), host memory reserve protections, and bounded output buffers.
     */
    [[nodiscard]] static ProcessResult execute(
        const std::string& binary,
        const std::vector<std::string>& args,
        const ProcessResourcePolicy& policy = {}
    );

    /**
     * @brief Probes active controller type on the current host (CgroupV2 or ProcessTreeMonitor).
     */
    [[nodiscard]] static ControllerType detect_best_controller();

    /**
     * @brief Helper to get RSS in bytes for a specific PID from /proc/<pid>/statm.
     */
    [[nodiscard]] static size_t get_process_rss_bytes(int pid);

    /**
     * @brief Helper to sum total RSS across an entire process tree / process group.
     */
    [[nodiscard]] static size_t get_process_tree_rss_bytes(int root_pid, std::vector<int>* out_pids = nullptr);
};

} // namespace nxdev::exec
