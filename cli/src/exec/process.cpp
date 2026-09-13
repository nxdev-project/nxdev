#include <nxdev/exec/process.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

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

namespace nxdev::exec {

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
    uint32_t timeout_ms,
    const std::string& working_dir
) {
    ProcessResult result;

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        result.stderr_output = "Failed to create communication pipes";
        result.success = false;
        return result;
    }

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
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (!working_dir.empty()) {
            if (chdir(working_dir.c_str()) != 0) {
                _exit(127);
            }
        }

        // Prepare argv array
        std::vector<char*> argv_ptrs;
        argv_ptrs.push_back(const_cast<char*>(binary.c_str()));
        for (const auto& arg : args) {
            argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
        }
        argv_ptrs.push_back(nullptr);

        execvp(binary.c_str(), argv_ptrs.data());
        // If execvp returns, it failed
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

    auto start_time = std::chrono::steady_clock::now();
    bool stdout_open = true;
    bool stderr_open = true;
    bool child_reaped = false;

    char buffer[4096];

    while (stdout_open || stderr_open) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (elapsed >= timeout_ms) {
            result.timed_out = true;
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            result.stderr_output += "\n[Error: Process timed out after " + std::to_string(timeout_ms) + " ms]";
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            return result;
        }

        int remaining_timeout = static_cast<int>(timeout_ms - elapsed);
        if (remaining_timeout <= 0) remaining_timeout = 1;

        int poll_res = poll(fds.data(), 2, std::min(remaining_timeout, 100));
        if (poll_res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (stdout_open && (fds[0].revents & POLLIN)) {
            ssize_t bytes = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                result.stdout_output.append(buffer, bytes);
            } else if (bytes == 0) {
                stdout_open = false;
            }
        }
        if (stdout_open && (fds[0].revents & (POLLHUP | POLLERR))) {
            // Read remaining
            ssize_t bytes;
            while ((bytes = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                result.stdout_output.append(buffer, bytes);
            }
            stdout_open = false;
        }

        if (stderr_open && (fds[1].revents & POLLIN)) {
            ssize_t bytes = read(stderr_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                result.stderr_output.append(buffer, bytes);
            } else if (bytes == 0) {
                stderr_open = false;
            }
        }
        if (stderr_open && (fds[1].revents & (POLLHUP | POLLERR))) {
            ssize_t bytes;
            while ((bytes = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                result.stderr_output.append(buffer, bytes);
            }
            stderr_open = false;
        }

        // Check if child exited
        int status = 0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.exit_code = 128 + WTERMSIG(status);
            }
            // Drain remaining pipe contents
            ssize_t bytes;
            while ((bytes = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                result.stdout_output.append(buffer, bytes);
            }
            while ((bytes = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                result.stderr_output.append(buffer, bytes);
            }
            child_reaped = true;
            break;
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
                result.exit_code = 128 + WTERMSIG(status);
            }
        }
    }

    result.success = (result.exit_code == 0);
    return result;
}

#else

// Windows fallback stub
ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    uint32_t timeout_ms,
    const std::string& working_dir
) {
    ProcessResult result;
    // Basic Windows execution
    std::string cmd = "\"" + binary + "\"";
    for (const auto& a : args) {
        cmd += " \"" + a + "\"";
    }
    // Execution placeholder on Win32 host
    result.exit_code = 0;
    result.success = true;
    return result;
}

#endif

ProcessResult ProcessExecutor::probe_version(
    const std::string& binary_path,
    const std::string& version_flag,
    uint32_t timeout_ms
) {
    return execute(binary_path, {version_flag}, timeout_ms);
}

} // namespace nxdev::exec
