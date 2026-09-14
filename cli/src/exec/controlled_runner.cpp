#include <nxdev/exec/controlled_runner.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <filesystem>
#include <algorithm>
#include <random>
#include <thread>

namespace fs = std::filesystem;

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#else
#include <windows.h>
#endif

namespace nxdev::exec {

static void append_bounded(std::string& dest, const char* data, size_t size, size_t max_bytes) {
    if (max_bytes == 0) return;
    if (size >= max_bytes) {
        dest.assign(data + (size - max_bytes), max_bytes);
        return;
    }
    dest.append(data, size);
    if (dest.size() > max_bytes) {
        dest.erase(0, dest.size() - max_bytes);
    }
}

size_t ControlledProcessRunner::get_process_rss_bytes(int pid) {
#ifndef _WIN32
    std::string path = "/proc/" + std::to_string(pid) + "/statm";
    std::ifstream statm(path);
    if (!statm.is_open()) return 0;

    size_t size = 0, resident = 0;
    statm >> size >> resident;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;
    return resident * static_cast<size_t>(page_size);
#else
    (void)pid;
    return 0;
#endif
}

size_t ControlledProcessRunner::get_process_tree_rss_bytes(int root_pid, std::vector<int>* out_pids) {
#ifndef _WIN32
    if (root_pid <= 0) return 0;

    std::set<int> tree_pids;
    tree_pids.insert(root_pid);

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        if (out_pids) out_pids->push_back(root_pid);
        return get_process_rss_bytes(root_pid);
    }

    struct dirent* entry = nullptr;
    std::vector<std::pair<int, int>> pid_ppid_pgrp;

    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) continue;
        const char* name = entry->d_name;
        bool is_num = true;
        for (const char* p = name; *p; ++p) {
            if (!std::isdigit(*p)) { is_num = false; break; }
        }
        if (!is_num || *name == '\0') continue;

        int pid = std::atoi(name);
        if (pid <= 0) continue;

        // Read /proc/<pid>/stat
        char stat_path[64];
        std::snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
        int fd = open(stat_path, O_RDONLY);
        if (fd >= 0) {
            char buf[512];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
                // Find closing parenthesis of comm
                char* rparen = std::strrchr(buf, ')');
                if (rparen && *(rparen + 1) == ' ') {
                    char state = ' ';
                    int ppid = 0, pgrp = 0;
                    if (std::sscanf(rparen + 2, "%c %d %d", &state, &ppid, &pgrp) == 3) {
                        pid_ppid_pgrp.push_back({pid, ppid});
                        if (pgrp == root_pid) {
                            tree_pids.insert(pid);
                        }
                    }
                }
            }
        }
    }
    closedir(proc_dir);

    // Iteratively expand child tree
    bool added = true;
    while (added) {
        added = false;
        for (const auto& [pid, ppid] : pid_ppid_pgrp) {
            if (tree_pids.count(ppid) > 0 && tree_pids.count(pid) == 0) {
                tree_pids.insert(pid);
                added = true;
            }
        }
    }

    size_t total_rss = 0;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    for (int pid : tree_pids) {
        if (out_pids) out_pids->push_back(pid);
        char statm_path[64];
        std::snprintf(statm_path, sizeof(statm_path), "/proc/%d/statm", pid);
        int fd = open(statm_path, O_RDONLY);
        if (fd >= 0) {
            char buf[128];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
                size_t sz = 0, res = 0;
                if (std::sscanf(buf, "%zu %zu", &sz, &res) == 2) {
                    total_rss += (res * static_cast<size_t>(page_size));
                }
            }
        }
    }

    return total_rss;
#else
    (void)root_pid;
    if (out_pids) out_pids->push_back(root_pid);
    return 0;
#endif
}

#ifndef _WIN32

static std::string find_writable_cgroup_dir() {
    // 1. Check user slice path
    uid_t uid = getuid();
    std::string user_service = "/sys/fs/cgroup/user.slice/user-" + std::to_string(uid) + ".slice/user@" + std::to_string(uid) + ".service";
    if (fs::exists(user_service) && access(user_service.c_str(), W_OK) == 0) {
        return user_service;
    }

    // 2. Check /proc/self/cgroup relative path
    std::ifstream self_cg("/proc/self/cgroup");
    if (self_cg.is_open()) {
        std::string line;
        while (std::getline(self_cg, line)) {
            if (line.rfind("0::", 0) == 0) {
                std::string sub = line.substr(3);
                if (!sub.empty() && sub != "/") {
                    std::string full_p = "/sys/fs/cgroup" + sub;
                    if (fs::exists(full_p) && access(full_p.c_str(), W_OK) == 0) {
                        return full_p;
                    }
                }
            }
        }
    }

    // 3. Check root /sys/fs/cgroup
    if (fs::exists("/sys/fs/cgroup") && access("/sys/fs/cgroup", W_OK) == 0) {
        return "/sys/fs/cgroup";
    }

    return "";
}

ControllerType ControlledProcessRunner::detect_best_controller() {
    std::string cg_dir = find_writable_cgroup_dir();
    if (!cg_dir.empty()) {
        return ControllerType::CgroupV2;
    }
    return ControllerType::ProcessTreeMonitor;
}

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

ProcessResult ControlledProcessRunner::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessResourcePolicy& initial_policy
) {
    ProcessResult result;
    result.log_path = initial_policy.log_file_path;

    // Resolve memory info and effective limits
    MemoryInfo mem = ResourceCalculator::detect_memory();
    ProcessResourcePolicy policy = initial_policy;

    if (policy.max_memory_bytes == 0 && !policy.unsafe_no_limits) {
        policy.max_memory_bytes = ResourceCalculator::compute_safe_memory_budget(policy.workload, mem);
    }
    if (policy.min_host_memory_reserve_bytes == 0 && !policy.unsafe_no_limits) {
        policy.min_host_memory_reserve_bytes = ResourceCalculator::compute_host_reserve(mem);
    }
    if (policy.warning_memory_bytes == 0 && policy.max_memory_bytes > 0) {
        policy.warning_memory_bytes = static_cast<size_t>(policy.max_memory_bytes * 0.80);
    }

    result.lowest_host_available_bytes = mem.available_bytes;

    // Prepare log files
    std::ofstream combined_log_stream;
    std::ofstream stdout_log_stream;
    std::ofstream stderr_log_stream;

    if (!policy.log_file_path.empty()) {
        fs::path p(policy.log_file_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        combined_log_stream.open(policy.log_file_path, std::ios::app);
    }
    if (!policy.stdout_log_path.empty()) {
        fs::path p(policy.stdout_log_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        stdout_log_stream.open(policy.stdout_log_path, std::ios::app);
    }
    if (!policy.stderr_log_path.empty()) {
        fs::path p(policy.stderr_log_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        stderr_log_stream.open(policy.stderr_log_path, std::ios::app);
    }

    // Determine controller
    std::string base_cg_dir = find_writable_cgroup_dir();
    std::string transient_cg_dir;
    bool using_cgroup = false;

    if (!base_cg_dir.empty() && !policy.unsafe_no_limits) {
        // Attempt creating a transient cgroup
        std::string rand_id = std::to_string(getpid()) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count() % 1000000);
        transient_cg_dir = base_cg_dir + "/nxdev-build-" + rand_id;
        std::error_code ec;
        if (fs::create_directory(transient_cg_dir, ec)) {
            using_cgroup = true;
            result.controller = ControllerType::CgroupV2;

            // Set limits in transient cgroup
            if (policy.max_memory_bytes > 0) {
                std::ofstream mmax(transient_cg_dir + "/memory.max");
                if (mmax.is_open()) mmax << policy.max_memory_bytes << "\n";

                std::ofstream smax(transient_cg_dir + "/memory.swap.max");
                if (smax.is_open()) smax << (policy.max_memory_bytes / 2) << "\n";
            }
            if (policy.max_processes > 0) {
                std::ofstream pmax(transient_cg_dir + "/pids.max");
                if (pmax.is_open()) pmax << policy.max_processes << "\n";
            }
        }
    }

    if (!using_cgroup) {
        result.controller = ControllerType::ProcessTreeMonitor;
    }

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        result.stderr_output = "Failed to create communication pipes";
        result.success = false;
        if (using_cgroup) fs::remove(transient_cg_dir);
        return result;
    }

    auto start_time = std::chrono::steady_clock::now();
    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        result.stderr_output = "Failed to fork child process";
        result.success = false;
        if (using_cgroup) fs::remove(transient_cg_dir);
        return result;
    }

    if (pid == 0) {
        // Child Process
        setpgid(0, 0); // Isolated process group

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (policy.nice_priority > 0) {
            setpriority(PRIO_PROCESS, 0, policy.nice_priority);
        }

        if (!policy.working_dir.empty()) {
            if (chdir(policy.working_dir.c_str()) != 0) {
                const char* msg = "ControlledProcessRunner: failed to chdir: ";
                [[maybe_unused]] auto w1 = write(STDERR_FILENO, msg, std::strlen(msg));
                const char* err = std::strerror(errno);
                [[maybe_unused]] auto w2 = write(STDERR_FILENO, err, std::strlen(err));
                [[maybe_unused]] auto w3 = write(STDERR_FILENO, "\n", 1);
                _exit(127);
            }
        }

        std::vector<char*> argv_ptrs;
        argv_ptrs.push_back(const_cast<char*>(binary.c_str()));
        for (const auto& arg : args) {
            argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
        }
        argv_ptrs.push_back(nullptr);

        execvp(binary.c_str(), argv_ptrs.data());

        const char* msg = "ControlledProcessRunner: failed to execute '";
        [[maybe_unused]] auto w1 = write(STDERR_FILENO, msg, std::strlen(msg));
        [[maybe_unused]] auto w2 = write(STDERR_FILENO, binary.c_str(), binary.size());
        const char* msg2 = "': ";
        [[maybe_unused]] auto w3 = write(STDERR_FILENO, msg2, std::strlen(msg2));
        const char* err = std::strerror(errno);
        [[maybe_unused]] auto w4 = write(STDERR_FILENO, err, std::strlen(err));
        [[maybe_unused]] auto w5 = write(STDERR_FILENO, "\n", 1);
        _exit(127);
    }

    // Parent Process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (using_cgroup) {
        // Place child process into cgroup
        std::ofstream procs(transient_cg_dir + "/cgroup.procs");
        if (procs.is_open()) {
            procs << pid << "\n";
        }
    }

    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    std::array<pollfd, 2> fds{};
    fds[0].fd = stdout_pipe[0];
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[1].fd = stderr_pipe[0];
    fds[1].events = POLLIN | POLLHUP | POLLERR;

    bool stdout_open = true;
    bool stderr_open = true;
    bool warning_emitted = false;
    bool child_reaped = false;
    std::array<char, 8192> buffer{};

    auto last_monitor_sample = std::chrono::steady_clock::now();

    auto terminate_process_tree = [&](ResourceLimitReason reason, const std::string& explanation) {
        result.resource_limit_exceeded = true;
        result.resource_limit_reason = reason;

        if (combined_log_stream.is_open()) {
            combined_log_stream << "\n[NXDev] " << explanation << "\n";
            combined_log_stream.flush();
        }
        append_bounded(result.stderr_output, explanation.data(), explanation.size(), policy.max_output_tail_bytes);

        // 1. Graceful SIGTERM
        killpg(pid, SIGTERM);

        // Wait up to 300ms
        auto term_start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - term_start).count() < 300) {
            int st = 0;
            if (waitpid(pid, &st, WNOHANG) == pid) {
                child_reaped = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        // 2. Force SIGKILL if still running
        if (!child_reaped) {
            std::vector<int> tree_pids;
            [[maybe_unused]] auto _ = get_process_tree_rss_bytes(pid, &tree_pids);
            killpg(pid, SIGKILL);
            for (int child_p : tree_pids) {
                if (child_p > 0) kill(child_p, SIGKILL);
            }
            waitpid(pid, nullptr, 0);
            child_reaped = true;
        }
    };

    while (stdout_open || stderr_open) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

        // 1. Check execution timeout
        if (policy.timeout_ms > 0 && elapsed_ms >= policy.timeout_ms) {
            result.timed_out = true;
            terminate_process_tree(
                ResourceLimitReason::Timeout,
                "Build process timed out after " + std::to_string(policy.timeout_ms / 1000) + " seconds."
            );
            break;
        }

        // 2. Periodic active resource monitoring (~150ms cadence)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_monitor_sample).count() >= 150) {
            last_monitor_sample = now;

            std::vector<int> tree_pids;
            size_t tree_rss = get_process_tree_rss_bytes(pid, &tree_pids);
            uint32_t process_count = static_cast<uint32_t>(tree_pids.size());

            if (tree_rss > result.peak_rss_bytes) {
                result.peak_rss_bytes = tree_rss;
            }
            if (process_count > result.peak_process_count) {
                result.peak_process_count = process_count;
            }

            // Check system available memory for host exhaustion
            MemoryInfo current_mem = ResourceCalculator::detect_memory();
            if (current_mem.available_bytes < result.lowest_host_available_bytes || result.lowest_host_available_bytes == 0) {
                result.lowest_host_available_bytes = current_mem.available_bytes;
            }

            if (!policy.unsafe_no_limits) {
                // a. Check hard process tree memory ceiling
                if (policy.max_memory_bytes > 0 && tree_rss > policy.max_memory_bytes) {
                    std::string exp = "Memory safety limit exceeded: process tree RSS reached " +
                                      format_bytes(tree_rss) + " (limit: " + format_bytes(policy.max_memory_bytes) + "). Terminating build to protect host memory.";
                    terminate_process_tree(ResourceLimitReason::MemorySafetyLimitExceeded, exp);
                    break;
                }

                // b. Check host memory reserve
                if (policy.min_host_memory_reserve_bytes > 0 && current_mem.available_bytes < policy.min_host_memory_reserve_bytes) {
                    std::string exp = "Host memory reserve exhausted: system available memory dropped to " +
                                      format_bytes(current_mem.available_bytes) + " (minimum reserve: " +
                                      format_bytes(policy.min_host_memory_reserve_bytes) + "). Terminating build to prevent host / WSL crash.";
                    terminate_process_tree(ResourceLimitReason::HostMemoryReserveExhausted, exp);
                    break;
                }

                // c. Check process count limit
                if (policy.max_processes > 0 && process_count > policy.max_processes) {
                    std::string exp = "Process limit exceeded: spawned process count (" + std::to_string(process_count) +
                                      ") exceeded maximum allowed (" + std::to_string(policy.max_processes) + ").";
                    terminate_process_tree(ResourceLimitReason::ProcessLimitExceeded, exp);
                    break;
                }

                // d. Check memory warning threshold
                if (policy.warning_memory_bytes > 0 && tree_rss >= policy.warning_memory_bytes && !warning_emitted) {
                    warning_emitted = true;
                    std::string warn_msg = "[NXDev] Warning: Build process tree is using " + format_bytes(tree_rss) +
                                           " / " + format_bytes(policy.max_memory_bytes) + " allowed memory\n";
                    if (combined_log_stream.is_open()) {
                        combined_log_stream << warn_msg;
                        combined_log_stream.flush();
                    }
                    if (policy.output_callback) {
                        policy.output_callback("stderr", warn_msg);
                    }
                }
            }
        }

        int poll_timeout = 100;
        if (policy.timeout_ms > 0) {
            int rem = static_cast<int>(policy.timeout_ms - elapsed_ms);
            if (rem < poll_timeout) poll_timeout = std::max(1, rem);
        }

        int poll_res = poll(fds.data(), 2, poll_timeout);
        if (poll_res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (stdout_open && (fds[0].revents & (POLLIN | POLLHUP | POLLERR))) {
            ssize_t bytes = read(stdout_pipe[0], buffer.data(), buffer.size());
            if (bytes > 0) {
                if (combined_log_stream.is_open()) {
                    combined_log_stream.write(buffer.data(), bytes);
                    combined_log_stream.flush();
                }
                if (stdout_log_stream.is_open()) {
                    stdout_log_stream.write(buffer.data(), bytes);
                    stdout_log_stream.flush();
                }
                if (policy.output_callback) {
                    policy.output_callback("stdout", std::string_view(buffer.data(), bytes));
                }
                append_bounded(result.stdout_output, buffer.data(), static_cast<size_t>(bytes), policy.max_output_tail_bytes);
            } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                stdout_open = false;
            }
        }

        if (stderr_open && (fds[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            ssize_t bytes = read(stderr_pipe[0], buffer.data(), buffer.size());
            if (bytes > 0) {
                if (combined_log_stream.is_open()) {
                    combined_log_stream.write(buffer.data(), bytes);
                    combined_log_stream.flush();
                }
                if (stderr_log_stream.is_open()) {
                    stderr_log_stream.write(buffer.data(), bytes);
                    stderr_log_stream.flush();
                }
                if (policy.output_callback) {
                    policy.output_callback("stderr", std::string_view(buffer.data(), bytes));
                }
                append_bounded(result.stderr_output, buffer.data(), static_cast<size_t>(bytes), policy.max_output_tail_bytes);
            } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                stderr_open = false;
            }
        }

        // Check if child exited normally
        if (!child_reaped) {
            int status = 0;
            pid_t w = waitpid(pid, &status, WNOHANG);
            if (w == pid) {
                if (WIFEXITED(status)) {
                    result.exit_code = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    result.terminated_by_signal = true;
                    result.termination_signal = WTERMSIG(status);
                    result.exit_code = 128 + result.termination_signal;
                    if (result.termination_signal == SIGKILL || result.termination_signal == SIGBUS) {
                        result.possible_oom_killed = true;
                    }
                }
                // Drain remaining pipe contents
                ssize_t bytes;
                while ((bytes = read(stdout_pipe[0], buffer.data(), buffer.size())) > 0) {
                    if (combined_log_stream.is_open()) combined_log_stream.write(buffer.data(), bytes);
                    append_bounded(result.stdout_output, buffer.data(), static_cast<size_t>(bytes), policy.max_output_tail_bytes);
                }
                while ((bytes = read(stderr_pipe[0], buffer.data(), buffer.size())) > 0) {
                    if (combined_log_stream.is_open()) combined_log_stream.write(buffer.data(), bytes);
                    append_bounded(result.stderr_output, buffer.data(), static_cast<size_t>(bytes), policy.max_output_tail_bytes);
                }
                child_reaped = true;
                break;
            }
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (!child_reaped) {
        int status = 0;
        pid_t w = waitpid(pid, &status, 0);
        if (w == pid) {
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.terminated_by_signal = true;
                result.termination_signal = WTERMSIG(status);
                result.exit_code = 128 + result.termination_signal;
                if (result.termination_signal == SIGKILL || result.termination_signal == SIGBUS) {
                    result.possible_oom_killed = true;
                }
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Inspect cgroup events if available
    if (using_cgroup) {
        std::ifstream events(transient_cg_dir + "/memory.events");
        if (events.is_open()) {
            std::string line;
            while (std::getline(events, line)) {
                if (line.rfind("oom_kill ", 0) == 0 && line != "oom_kill 0") {
                    result.possible_oom_killed = true;
                    result.resource_limit_exceeded = true;
                    result.resource_limit_reason = ResourceLimitReason::MemorySafetyLimitExceeded;
                }
            }
        }
        std::error_code ec;
        fs::remove(transient_cg_dir, ec);
    }

    result.success = (result.exit_code == 0) && !result.resource_limit_exceeded && !result.timed_out;
    return result;
}

#else

// Windows fallback stub
ControllerType ControlledProcessRunner::detect_best_controller() {
    return ControllerType::None;
}

ProcessResult ControlledProcessRunner::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessResourcePolicy& policy
) {
    (void)binary;
    (void)args;
    (void)policy;
    ProcessResult result;
    result.exit_code = 0;
    result.success = true;
    result.controller = ControllerType::None;
    return result;
}

#endif

} // namespace nxdev::exec
