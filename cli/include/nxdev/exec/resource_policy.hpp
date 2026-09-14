#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <optional>
#include <functional>

namespace nxdev::exec {

enum class WorkloadType {
    Generic,
    BuildApplication,
    BuildThirdPartyBackend,
    RunPackagingBackend
};

enum class ControllerType {
    None,
    ProcessTreeMonitor,
    CgroupV2
};

enum class ResourceLimitReason {
    None,
    MemorySafetyLimitExceeded,
    HostMemoryReserveExhausted,
    ProcessLimitExceeded,
    Timeout,
    SwapPressureExceeded
};

struct ProcessResourcePolicy {
    WorkloadType workload{WorkloadType::Generic};
    size_t max_memory_bytes{0};              // 0 = automatic / policy-calculated
    size_t warning_memory_bytes{0};          // 0 = 80% of max_memory_bytes
    size_t min_host_memory_reserve_bytes{0}; // 0 = automatic (e.g., max(768MB, 20% of RAM))
    uint32_t max_processes{64};              // pids.max limit
    uint32_t max_threads{0};                 // 0 = unbounded / env default
    uint32_t timeout_ms{600000};             // 10 minutes default for builds
    int nice_priority{0};                    // 0 = normal, positive = lower priority
    size_t max_output_tail_bytes{65536};     // 64 KiB bounded tail buffer
    bool unsafe_no_limits{false};            // Explicit unsafe override bypass
    std::string working_dir;
    std::string log_file_path;
    std::string stdout_log_path;
    std::string stderr_log_path;
    std::function<void(std::string_view stream, std::string_view chunk)> output_callback;
};

struct ProcessResourceUsage {
    size_t peak_rss_bytes{0};
    uint32_t peak_process_count{1};
    size_t lowest_host_available_bytes{0};
    uint64_t duration_ms{0};
    ControllerType controller{ControllerType::None};
};

struct MemoryInfo {
    size_t total_bytes{0};
    size_t available_bytes{0};
    size_t free_bytes{0};
    size_t swap_total_bytes{0};
    size_t swap_free_bytes{0};
    size_t cgroup_max_bytes{0};
    bool is_wsl{false};
};

// Parser helpers
std::optional<size_t> parse_memory_size_string(std::string_view str);
std::string format_bytes(size_t bytes);
std::string controller_type_to_string(ControllerType type);
std::string resource_limit_reason_to_string(ResourceLimitReason reason);

} // namespace nxdev::exec
