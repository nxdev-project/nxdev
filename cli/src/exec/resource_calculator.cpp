#include <nxdev/exec/resource_calculator.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace nxdev::exec {

std::optional<size_t> parse_memory_size_string(std::string_view str) {
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front()))) str.remove_prefix(1);
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) str.remove_suffix(1);

    if (str.empty()) return std::nullopt;

    std::string lower(str);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

    if (lower == "auto") return 0;
    if (lower == "unlimited" || lower == "none" || lower == "inf") return static_cast<size_t>(-1);

    // Extract numeric portion and unit suffix
    size_t i = 0;
    while (i < lower.size() && (std::isdigit(static_cast<unsigned char>(lower[i])) || lower[i] == '.')) {
        i++;
    }

    if (i == 0) return std::nullopt;

    double num = 0.0;
    try {
        num = std::stod(lower.substr(0, i));
    } catch (...) {
        return std::nullopt;
    }

    if (num < 0) return std::nullopt;

    std::string unit = lower.substr(i);
    while (!unit.empty() && std::isspace(static_cast<unsigned char>(unit.front()))) unit.erase(0, 1);

    double multiplier = 1.0;
    if (unit.empty() || unit == "b" || unit == "bytes") {
        multiplier = 1.0;
    } else if (unit == "k" || unit == "kb" || unit == "kib") {
        multiplier = 1024.0;
    } else if (unit == "m" || unit == "mb" || unit == "mib") {
        multiplier = 1024.0 * 1024.0;
    } else if (unit == "g" || unit == "gb" || unit == "gib") {
        multiplier = 1024.0 * 1024.0 * 1024.0;
    } else if (unit == "t" || unit == "tb" || unit == "tib") {
        multiplier = 1024.0 * 1024.0 * 1024.0 * 1024.0;
    } else {
        return std::nullopt; // Unknown unit
    }

    return static_cast<size_t>(num * multiplier);
}

std::string format_bytes(size_t bytes) {
    if (bytes == static_cast<size_t>(-1)) return "unlimited";
    if (bytes == 0) return "0 B";

    const char* suffixes[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double count = static_cast<double>(bytes);
    int s = 0;
    while (count >= 1024.0 && s < 4) {
        count /= 1024.0;
        s++;
    }

    char buf[64];
    if (s == 0) {
        std::snprintf(buf, sizeof(buf), "%zu %s", bytes, suffixes[s]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.2f %s", count, suffixes[s]);
    }
    return std::string(buf);
}

std::string controller_type_to_string(ControllerType type) {
    switch (type) {
        case ControllerType::CgroupV2: return "cgroup v2";
        case ControllerType::ProcessTreeMonitor: return "process-tree monitor";
        case ControllerType::None: return "none";
    }
    return "none";
}

std::string resource_limit_reason_to_string(ResourceLimitReason reason) {
    switch (reason) {
        case ResourceLimitReason::None: return "None";
        case ResourceLimitReason::MemorySafetyLimitExceeded: return "Memory safety limit exceeded";
        case ResourceLimitReason::HostMemoryReserveExhausted: return "Host memory reserve exhausted";
        case ResourceLimitReason::ProcessLimitExceeded: return "Process limit exceeded";
        case ResourceLimitReason::Timeout: return "Execution timed out";
        case ResourceLimitReason::SwapPressureExceeded: return "Swap pressure exceeded";
    }
    return "Unknown";
}

MemoryInfo ResourceCalculator::detect_memory() {
    MemoryInfo info;

#ifndef _WIN32
    // 1. Check WSL
    if (std::getenv("WSL_DISTRO_NAME") != nullptr || std::getenv("WSL_INTEROP") != nullptr) {
        info.is_wsl = true;
    } else {
        std::ifstream vfile("/proc/version");
        if (vfile.is_open()) {
            std::string line;
            std::getline(vfile, line);
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
            if (lower.find("microsoft") != std::string::npos || lower.find("wsl") != std::string::npos) {
                info.is_wsl = true;
            }
        }
    }

    // 2. Parse /proc/meminfo
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                size_t kb = 0;
                std::istringstream(line.substr(9)) >> kb;
                info.total_bytes = kb * 1024;
            } else if (line.rfind("MemFree:", 0) == 0) {
                size_t kb = 0;
                std::istringstream(line.substr(8)) >> kb;
                info.free_bytes = kb * 1024;
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                size_t kb = 0;
                std::istringstream(line.substr(13)) >> kb;
                info.available_bytes = kb * 1024;
            } else if (line.rfind("SwapTotal:", 0) == 0) {
                size_t kb = 0;
                std::istringstream(line.substr(10)) >> kb;
                info.swap_total_bytes = kb * 1024;
            } else if (line.rfind("SwapFree:", 0) == 0) {
                size_t kb = 0;
                std::istringstream(line.substr(9)) >> kb;
                info.swap_free_bytes = kb * 1024;
            }
        }
    }

    if (info.available_bytes == 0 && info.free_bytes > 0) {
        info.available_bytes = info.free_bytes;
    }

    // 3. Check cgroup memory max if present
    std::ifstream cg_max("/sys/fs/cgroup/memory.max");
    if (cg_max.is_open()) {
        std::string val;
        cg_max >> val;
        if (val != "max" && !val.empty()) {
            try {
                info.cgroup_max_bytes = std::stoull(val);
                if (info.cgroup_max_bytes > 0 && info.cgroup_max_bytes < info.total_bytes) {
                    info.total_bytes = info.cgroup_max_bytes;
                }
            } catch (...) {}
        }
    }
#else
    // Fallback default
    info.total_bytes = 8ULL * 1024 * 1024 * 1024;
    info.available_bytes = 4ULL * 1024 * 1024 * 1024;
#endif

    return info;
}

size_t ResourceCalculator::compute_host_reserve(const MemoryInfo& mem) {
    if (mem.total_bytes == 0) {
        return 1024ULL * 1024 * 1024; // 1 GiB fallback
    }

    // On WSL or native Linux: Reserve at least 20-25% of total memory or minimum 768 MiB (1 GiB on >= 4 GiB systems)
    size_t min_floor = (mem.total_bytes >= 4ULL * 1024 * 1024 * 1024)
        ? (1024ULL * 1024 * 1024)   // 1 GiB
        : (768ULL * 1024 * 1024);    // 768 MiB

    size_t fraction = static_cast<size_t>(mem.total_bytes * 0.20);
    return std::max(min_floor, fraction);
}

size_t ResourceCalculator::compute_safe_memory_budget(
    WorkloadType workload,
    const MemoryInfo& mem,
    size_t explicit_limit
) {
    if (explicit_limit > 0 && explicit_limit != static_cast<size_t>(-1)) {
        return explicit_limit;
    }

    size_t available = mem.available_bytes > 0 ? mem.available_bytes : mem.free_bytes;
    size_t reserve = compute_host_reserve(mem);

    if (available <= reserve) {
        return 0; // Host is dangerously low on memory
    }

    size_t budget = available - reserve;

    // Impose sensible workload-specific bounds
    if (workload == WorkloadType::BuildThirdPartyBackend) {
        // e.g., hacBrewPack build: conservative ceiling of at most 75% of total memory or 4 GiB
        budget = std::min(budget, static_cast<size_t>(mem.total_bytes * 0.75));
        budget = std::min(budget, static_cast<size_t>(4ULL * 1024 * 1024 * 1024));
    } else if (workload == WorkloadType::RunPackagingBackend) {
        // Packaging runtime: cap at 4 GiB or safe fraction
        budget = std::min(budget, static_cast<size_t>(4ULL * 1024 * 1024 * 1024));
    } else if (workload == WorkloadType::BuildApplication) {
        // Application build: cap at 85% of total memory
        budget = std::min(budget, static_cast<size_t>(mem.total_bytes * 0.85));
    }

    return budget;
}

uint32_t ResourceCalculator::compute_safe_job_count(
    WorkloadType workload,
    const MemoryInfo& mem,
    size_t budget_bytes,
    std::optional<uint32_t> requested_jobs,
    bool unsafe_override,
    bool* was_clamped
) {
    (void)mem;
    if (was_clamped) *was_clamped = false;

    // For third party backend compilation (hacBrewPack): default jobs = 1 strictly
    if (workload == WorkloadType::BuildThirdPartyBackend) {
        if (requested_jobs.has_value() && unsafe_override) {
            return std::max(uint32_t(1), *requested_jobs);
        }
        return 1;
    }

    // Determine CPU capacity
    uint32_t cpus = std::thread::hardware_concurrency();
    if (cpus == 0) cpus = 2;

    // Compute memory capacity: 1.5 GiB per job
    uint32_t mem_jobs = static_cast<uint32_t>(budget_bytes / ESTIMATED_MEMORY_PER_COMPILER_JOB);
    if (mem_jobs == 0) mem_jobs = 1;

    uint32_t safe_jobs = std::max(uint32_t(1), std::min(cpus, mem_jobs));

    if (requested_jobs.has_value()) {
        if (unsafe_override || *requested_jobs <= safe_jobs) {
            return std::max(uint32_t(1), *requested_jobs);
        }
        if (was_clamped) *was_clamped = true;
        return safe_jobs;
    }

    return safe_jobs;
}

void ResourceCalculator::sanitize_environment(uint32_t safe_jobs) {
#ifndef _WIN32
    // Set controlled CMake parallel level
    std::string jobs_str = std::to_string(safe_jobs);
    setenv("CMAKE_BUILD_PARALLEL_LEVEL", jobs_str.c_str(), 1);

    // Sanitize MAKEFLAGS to prevent hidden -j64
    const char* makeflags = std::getenv("MAKEFLAGS");
    if (makeflags) {
        std::string mf(makeflags);
        // Replace or strip unsafe -j flags
        std::string sanitized = "-j" + jobs_str;
        setenv("MAKEFLAGS", sanitized.c_str(), 1);
    } else {
        std::string sanitized = "-j" + jobs_str;
        setenv("MAKEFLAGS", sanitized.c_str(), 1);
    }

    // Set OMP_NUM_THREADS conservatively to match safe jobs
    setenv("OMP_NUM_THREADS", jobs_str.c_str(), 1);
    setenv("LLVM_PARALLEL_LINK_JOBS", "1", 1);
#else
    (void)safe_jobs;
#endif
}

} // namespace nxdev::exec
