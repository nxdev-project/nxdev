#include "test_common.hpp"
#include <nxdev/exec/resource_policy.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <nxdev/exec/controlled_runner.hpp>
#include <iostream>
#include <vector>
#include <string>

using namespace nxdev::exec;

static void test_string_parser() {
    std::cout << "[Test] Memory Size String Parser...\n";

    // Valid inputs
    auto b1 = parse_memory_size_string("512M");
    NXDEV_TEST_ASSERT(b1.has_value() && *b1 == 512ULL * 1024 * 1024);

    auto b2 = parse_memory_size_string("2G");
    NXDEV_TEST_ASSERT(b2.has_value() && *b2 == 2ULL * 1024 * 1024 * 1024);

    auto b3 = parse_memory_size_string("4096MB");
    NXDEV_TEST_ASSERT(b3.has_value() && *b3 == 4096ULL * 1024 * 1024);

    auto b4 = parse_memory_size_string("1.5GiB");
    NXDEV_TEST_ASSERT(b4.has_value() && *b4 == static_cast<size_t>(1.5 * 1024 * 1024 * 1024));

    auto b5 = parse_memory_size_string("auto");
    NXDEV_TEST_ASSERT(b5.has_value() && *b5 == 0);

    auto b6 = parse_memory_size_string("unlimited");
    NXDEV_TEST_ASSERT(b6.has_value() && *b6 == static_cast<size_t>(-1));

    auto b7 = parse_memory_size_string("  1024 K  ");
    NXDEV_TEST_ASSERT(b7.has_value() && *b7 == 1024 * 1024);

    // Invalid inputs
    NXDEV_TEST_ASSERT(!parse_memory_size_string("").has_value());
    NXDEV_TEST_ASSERT(!parse_memory_size_string("invalid").has_value());
    NXDEV_TEST_ASSERT(!parse_memory_size_string("512XYZ").has_value());
    NXDEV_TEST_ASSERT(!parse_memory_size_string("-10M").has_value());
}

static void test_format_bytes() {
    std::cout << "[Test] Format Bytes...\n";
    NXDEV_TEST_ASSERT(format_bytes(512) == "512 B");
    NXDEV_TEST_ASSERT(format_bytes(1024 * 1024) == "1.00 MiB");
    NXDEV_TEST_ASSERT(format_bytes(4ULL * 1024 * 1024 * 1024) == "4.00 GiB");
    NXDEV_TEST_ASSERT(format_bytes(0) == "0 B");
    NXDEV_TEST_ASSERT(format_bytes(static_cast<size_t>(-1)) == "unlimited");
}

static void test_budget_and_job_calculation() {
    std::cout << "[Test] Budget and Safe Job Calculation...\n";

    MemoryInfo mem;
    mem.total_bytes = 8ULL * 1024 * 1024 * 1024;    // 8 GiB
    mem.available_bytes = 6ULL * 1024 * 1024 * 1024; // 6 GiB

    size_t reserve = ResourceCalculator::compute_host_reserve(mem);
    NXDEV_TEST_ASSERT(reserve >= 1024ULL * 1024 * 1024); // At least 1 GiB for 8 GiB system

    // Application budget
    size_t app_budget = ResourceCalculator::compute_safe_memory_budget(WorkloadType::BuildApplication, mem);
    NXDEV_TEST_ASSERT(app_budget > 0);
    NXDEV_TEST_ASSERT(app_budget <= mem.available_bytes - reserve);

    // Third party backend build (hacBrewPack): compile jobs = 1 strictly
    uint32_t backend_jobs = ResourceCalculator::compute_safe_job_count(WorkloadType::BuildThirdPartyBackend, mem, app_budget);
    NXDEV_TEST_ASSERT(backend_jobs == 1);

    // Application build jobs
    uint32_t app_jobs = ResourceCalculator::compute_safe_job_count(WorkloadType::BuildApplication, mem, app_budget);
    NXDEV_TEST_ASSERT(app_jobs >= 1);

    // Clamping test when requesting unsafe huge parallelism
    bool was_clamped = false;
    uint32_t clamped_jobs = ResourceCalculator::compute_safe_job_count(WorkloadType::BuildApplication, mem, app_budget, 64, false, &was_clamped);
    NXDEV_TEST_ASSERT(was_clamped);
    NXDEV_TEST_ASSERT(clamped_jobs == app_jobs);

    // Unsafe override test
    bool was_clamped_unsafe = false;
    uint32_t unsafe_jobs = ResourceCalculator::compute_safe_job_count(WorkloadType::BuildApplication, mem, app_budget, 64, true, &was_clamped_unsafe);
    NXDEV_TEST_ASSERT(!was_clamped_unsafe);
    NXDEV_TEST_ASSERT(unsafe_jobs == 64);

    // Low memory exhaustion scenario
    MemoryInfo exhausted_mem;
    exhausted_mem.total_bytes = 4ULL * 1024 * 1024 * 1024;
    exhausted_mem.available_bytes = 512ULL * 1024 * 1024; // 512 MiB available < 1 GiB reserve
    size_t zero_budget = ResourceCalculator::compute_safe_memory_budget(WorkloadType::BuildApplication, exhausted_mem);
    NXDEV_TEST_ASSERT(zero_budget == 0);
}

static void test_controlled_runner_basic() {
    std::cout << "[Test] Controlled Process Runner Basic Execution...\n";

    ProcessResourcePolicy policy;
    policy.timeout_ms = 5000;

    auto res = ControlledProcessRunner::execute("echo", {"hello", "nxdev"}, policy);
    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.exit_code == 0);
    NXDEV_TEST_ASSERT(res.stdout_output.find("hello nxdev") != std::string::npos);
    NXDEV_TEST_ASSERT(!res.resource_limit_exceeded);
    NXDEV_TEST_ASSERT(!res.timed_out);
}

static void test_process_tree_monitoring() {
    std::cout << "[Test] Process Tree Monitoring & Child PID Tracking...\n";

    // Spawn a shell command that launches multiple sub-processes
    ProcessResourcePolicy policy;
    policy.timeout_ms = 5000;

    auto res = ControlledProcessRunner::execute(
        "sh",
        {"-c", "sleep 0.1 & sleep 0.1 & wait"},
        policy
    );

    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.exit_code == 0);
}

static void test_memory_hog_termination() {
    std::cout << "[Test] Memory-Hog Regression Test (Safe Kill on Limit Exceeded)...\n";

    // Set a tiny memory limit of 15 MiB
    ProcessResourcePolicy policy;
    policy.max_memory_bytes = 15ULL * 1024 * 1024;
    policy.timeout_ms = 5000;

    // Helper runs a python/perl/sh one-liner or dd allocating 60 MiB
    // Using dd to read into head/cat or python3 if available
    auto res = ControlledProcessRunner::execute(
        "sh",
        {"-c", "python3 -c 'import time; a = bytearray(80*1024*1024); time.sleep(1)' 2>/dev/null || (head -c 80000000 /dev/zero | tail -c 80000000)"},
        policy
    );

    NXDEV_TEST_ASSERT(!res.success);
    NXDEV_TEST_ASSERT(res.resource_limit_exceeded);
    NXDEV_TEST_ASSERT(res.resource_limit_reason == ResourceLimitReason::MemorySafetyLimitExceeded);
    NXDEV_TEST_ASSERT(res.stderr_output.find("Memory safety limit exceeded") != std::string::npos);
}

static void test_process_count_limit() {
    std::cout << "[Test] Process-Count Limit Regression Test...\n";

    // Set max processes = 3
    ProcessResourcePolicy policy;
    policy.max_processes = 3;
    policy.timeout_ms = 5000;

    auto res = ControlledProcessRunner::execute(
        "sh",
        {"-c", "sleep 2 & sleep 2 & sleep 2 & sleep 2 & sleep 2 & sleep 2 & sleep 2 & sleep 2 & wait"},
        policy
    );

    NXDEV_TEST_ASSERT(!res.success);
    NXDEV_TEST_ASSERT(res.resource_limit_exceeded);
    NXDEV_TEST_ASSERT(res.resource_limit_reason == ResourceLimitReason::ProcessLimitExceeded);
}

static void test_bounded_output_buffer() {
    std::cout << "[Test] Bounded Output Buffer (No Unbounded RAM Growth)...\n";

    ProcessResourcePolicy policy;
    policy.max_output_tail_bytes = 4096; // 4 KiB tail
    policy.timeout_ms = 5000;

    // Emit 2 MiB of output
    auto res = ControlledProcessRunner::execute(
        "sh",
        {"-c", "head -c 2000000 /dev/zero | tr '\\0' 'A'"},
        policy
    );

    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.stdout_output.size() <= 4096);
}

static void test_timeout_cancellation() {
    std::cout << "[Test] Timeout Cancellation of Process Tree...\n";

    ProcessResourcePolicy policy;
    policy.timeout_ms = 400; // 400ms timeout

    auto res = ControlledProcessRunner::execute(
        "sh",
        {"-c", "sleep 5 & sleep 5 & wait"},
        policy
    );

    NXDEV_TEST_ASSERT(!res.success);
    NXDEV_TEST_ASSERT(res.timed_out);
}

int main() {
    std::cout << "=== Running NXDev Resource Control Tests ===\n\n";

    test_string_parser();
    test_format_bytes();
    test_budget_and_job_calculation();
    test_controlled_runner_basic();
    test_process_tree_monitoring();
    test_memory_hog_termination();
    test_process_count_limit();
    test_bounded_output_buffer();
    test_timeout_cancellation();

    std::cout << "\n[PASS] All Resource Control Tests passed successfully!\n";
    return 0;
}
