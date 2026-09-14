#include <nxdev/pack/process.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
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
            size_t cur_rss = get_process_rss_bytes(pid);
            if (cur_rss > result.peak_rss_bytes) {
                result.peak_rss_bytes = cur_rss;
            }

            if (options.max_memory_bytes > 0 && cur_rss > options.max_memory_bytes) {
                result.memory_limit_exceeded = true;
                killpg(pid, SIGKILL);
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
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    auto end_time = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
        result.success = (result.exit_code == 0);
    } else if (WIFSIGNALED(status)) {
        result.termination_signal = WTERMSIG(status);
        result.terminated_by_signal = true;
        result.exit_code = 128 + result.termination_signal;
        result.success = false;

        if (result.termination_signal == SIGKILL && !result.timed_out && !result.memory_limit_exceeded) {
            result.possible_oom_killed = true;
        }
    }

    if (combined_log_stream.is_open()) combined_log_stream.close();
    if (stdout_log_stream.is_open()) stdout_log_stream.close();
    if (stderr_log_stream.is_open()) stderr_log_stream.close();

    return result;
}

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

#else

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessOptions& options
) {
    ProcessResult result;
    std::string cmd = "\"" + binary + "\"";
    for (const auto& a : args) {
        cmd += " \"" + a + "\"";
    }
    int code = std::system(cmd.c_str());
    result.exit_code = code;
    result.success = (code == 0);
    return result;
}

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

#endif

} // namespace nxdev::pack
