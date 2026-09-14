#pragma once

#include <nxdev/exec/resource_policy.hpp>
#include <optional>
#include <string>

namespace nxdev::exec {

class ResourceCalculator {
public:
    /**
     * @brief Detects system memory details from /proc/meminfo and cgroup v2.
     */
    [[nodiscard]] static MemoryInfo detect_memory();

    /**
     * @brief Computes the memory reserved for the host/WSL kernel and system processes.
     */
    [[nodiscard]] static size_t compute_host_reserve(const MemoryInfo& mem);

    /**
     * @brief Computes safe memory budget for a child build or execution process.
     */
    [[nodiscard]] static size_t compute_safe_memory_budget(
        WorkloadType workload,
        const MemoryInfo& mem,
        size_t explicit_limit = 0
    );

    /**
     * @brief Computes resource-aware parallel job count.
     */
    [[nodiscard]] static uint32_t compute_safe_job_count(
        WorkloadType workload,
        const MemoryInfo& mem,
        size_t budget_bytes,
        std::optional<uint32_t> requested_jobs = std::nullopt,
        bool unsafe_override = false,
        bool* was_clamped = nullptr
    );

    /**
     * @brief Sanitizes environment variables (MAKEFLAGS, NINJAFLAGS, CMAKE_BUILD_PARALLEL_LEVEL, etc.)
     * to prevent child builds from silently inheriting unsafe parallelism settings.
     */
    static void sanitize_environment(uint32_t safe_jobs);

    /**
     * @brief Minimum viable memory budget required to start a build safely.
     */
    static constexpr size_t MINIMUM_VIABLE_BUILD_BUDGET = 384ULL * 1024 * 1024; // 384 MiB
    static constexpr size_t ESTIMATED_MEMORY_PER_COMPILER_JOB = 1536ULL * 1024 * 1024; // 1.5 GiB
};

} // namespace nxdev::exec
