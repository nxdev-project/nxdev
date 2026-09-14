#include <nxdev/pack/process.hpp>
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
#include <thread>

namespace fs = std::filesystem;

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#else
#include <windows.h>
#endif

namespace nxdev::pack {

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

size_t ProcessExecutor::get_available_memory_bytes() {
#ifndef _WIN32
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) return 0;

    std::string line;
    size_t mem_available_kb = 0;
    size_t mem_free_kb = 0;
    while (std::getline(meminfo, line)) {
        if (line.rfind("MemAvailable:", 0) == 0) {
            std::istringstream iss(line.substr(13));
            iss >> mem_available_kb;
            return mem_available_kb * 1024;
        } else if (line.rfind("MemFree:", 0) == 0) {
            std::istringstream iss(line.substr(8));
            iss >> mem_free_kb;
        }
    }
    return mem_free_kb * 1024;
#else
    return 0;
#endif
}

size_t ProcessExecutor::get_process_rss_bytes(int pid) {
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

#ifndef _WIN32
static size_t get_tree_rss_bytes(int root_pid, std::vector<int>* out_pids = nullptr) {
    if (root_pid <= 0) return 0;

    std::set<int> tree_pids;
    tree_pids.insert(root_pid);

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        if (out_pids) out_pids->push_back(root_pid);
        return ProcessExecutor::get_process_rss_bytes(root_pid);
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

        char stat_path[64];
        std::snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
        int fd = open(stat_path, O_RDONLY);
        if (fd >= 0) {
            char buf[512];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
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
}
#endif

bool ProcessExecutor::is_wsl_environment() {
#ifndef _WIN32
    if (std::getenv("WSL_DISTRO_NAME") != nullptr || std::getenv("WSL_INTEROP") != nullptr) {
        return true;
    }
    std::ifstream version("/proc/version");
    if (version.is_open()) {
        std::string line;
        std::getline(version, line);
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (lower.find("microsoft") != std::string::npos || lower.find("wsl") != std::string::npos) {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

#ifndef _WIN32

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessOptions& options
) {
    ProcessResult result;
    result.log_path = options.log_file_path;

    // Prepare log files if requested
    std::ofstream combined_log_stream;
    std::ofstream stdout_log_stream;
    std::ofstream stderr_log_stream;

    if (!options.log_file_path.empty()) {
        fs::path p(options.log_file_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        combined_log_stream.open(options.log_file_path, std::ios::app);
    }
    if (!options.stdout_log_path.empty()) {
        fs::path p(options.stdout_log_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        stdout_log_stream.open(options.stdout_log_path, std::ios::app);
    }
    if (!options.stderr_log_path.empty()) {
        fs::path p(options.stderr_log_path);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        stderr_log_stream.open(options.stderr_log_path, std::ios::app);
    }

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        result.stderr_output = "Failed to create communication pipes";
        result.success = false;
        return result;
    }

    auto start_time = std::chrono::steady_clock::now();
    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        result.stderr_output = "Failed to fork child process";
        result.success = false;
        return result;
    }

    if (pid == 0) {
        // Child Process
        // Set new process group so child process tree can be cleaned up reliably
        setpgid(0, 0);

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (!options.working_dir.empty()) {
            if (chdir(options.working_dir.c_str()) != 0) {
                const char* msg = "ProcessExecutor: failed to change directory: ";
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

        const char* msg = "ProcessExecutor: failed to execute '";
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

    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    std::array<pollfd, 2> fds{};
    fds[0].fd = stdout_pipe[0];
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[1].fd = stderr_pipe[0];
    fds[1].events = POLLIN | POLLHUP | POLLERR;

    bool stdout_open = true;
    bool stderr_open = true;
    bool child_reaped = false;
    std::array<char, 8192> buffer{};

    auto last_rss_sample = std::chrono::steady_clock::now();

    while (stdout_open || stderr_open) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

        // Check timeout
        if (options.timeout_ms > 0 && elapsed_ms >= options.timeout_ms) {
            result.timed_out = true;
            killpg(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            result.exit_code = -1;
            std::string timeout_msg = "\nProcess timed out after " + std::to_string(options.timeout_ms) + " ms";
            append_bounded(result.stderr_output, timeout_msg.data(), timeout_msg.size(), options.max_output_tail_bytes);
            break;
        }

        // Periodic RSS and resource monitor check (every ~100ms)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_rss_sample).count() >= 100) {
            last_rss_sample = now;
            std::vector<int> tree_pids;
            size_t cur_rss = get_tree_rss_bytes(pid, &tree_pids);
            if (cur_rss > result.peak_rss_bytes) {
                result.peak_rss_bytes = cur_rss;
            }

            if (options.max_memory_bytes > 0 && cur_rss > options.max_memory_bytes) {
                result.memory_limit_exceeded = true;
                killpg(pid, SIGTERM);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                killpg(pid, SIGKILL);
                for (int p : tree_pids) { if (p > 0) kill(p, SIGKILL); }
                waitpid(pid, nullptr, 0);
                std::string msg = "\nProcess exceeded memory safety ceiling of " +
                                  std::to_string(options.max_memory_bytes / (1024 * 1024)) +
                                  " MiB (current RSS: " + std::to_string(cur_rss / (1024 * 1024)) + " MiB)\n";
                if (combined_log_stream.is_open()) {
                    combined_log_stream << msg;
                    combined_log_stream.flush();
                }
                append_bounded(result.stderr_output, msg.data(), msg.size(), options.max_output_tail_bytes);
                break;
            }
        }

        int poll_timeout = 100;
        if (options.timeout_ms > 0) {
            int rem = static_cast<int>(options.timeout_ms - elapsed_ms);
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
                if (options.output_callback) {
                    options.output_callback("stdout", std::string_view(buffer.data(), bytes));
                }
                append_bounded(result.stdout_output, buffer.data(), static_cast<size_t>(bytes), options.max_output_tail_bytes);
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
                if (options.output_callback) {
                    options.output_callback("stderr", std::string_view(buffer.data(), bytes));
                }
                append_bounded(result.stderr_output, buffer.data(), static_cast<size_t>(bytes), options.max_output_tail_bytes);
            } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                stderr_open = false;
            }
        }

        // Check if child exited
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
                    append_bounded(result.stdout_output, buffer.data(), static_cast<size_t>(bytes), options.max_output_tail_bytes);
                }
                while ((bytes = read(stderr_pipe[0], buffer.data(), buffer.size())) > 0) {
                    if (combined_log_stream.is_open()) combined_log_stream.write(buffer.data(), bytes);
                    append_bounded(result.stderr_output, buffer.data(), static_cast<size_t>(bytes), options.max_output_tail_bytes);
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
    result.success = (result.exit_code == 0) && !result.memory_limit_exceeded && !result.timed_out;
    return result;
}

#else

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessOptions& options
) {
    (void)binary;
    (void)args;
    (void)options;
    ProcessResult result;
    result.exit_code = 0;
    result.success = true;
    return result;
}

#endif

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    uint32_t timeout_ms,
    const std::string& working_dir
) {
    ProcessOptions opt;
    opt.timeout_ms = timeout_ms;
    opt.working_dir = working_dir;
    return execute(binary, args, opt);
}

} // namespace nxdev::pack
