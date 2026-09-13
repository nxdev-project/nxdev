#include <nxdev/pack/process.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

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

        std::vector<char*> argv_ptrs;
        argv_ptrs.push_back(const_cast<char*>(binary.c_str()));
        for (const auto& arg : args) {
            argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
        }
        argv_ptrs.push_back(nullptr);

        execvp(binary.c_str(), argv_ptrs.data());
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
    std::array<char, 4096> buffer{};

    while (stdout_open || stderr_open) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        if (timeout_ms > 0 && elapsed >= timeout_ms) {
            result.timed_out = true;
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            result.exit_code = -1;
            result.success = false;
            result.stderr_output += "\nProcess timed out after " + std::to_string(timeout_ms) + " ms";
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            return result;
        }

        int remaining_ms = timeout_ms > 0 ? static_cast<int>(timeout_ms - elapsed) : 100;
        int poll_res = poll(fds.data(), 2, std::min(remaining_ms, 100));

        if (poll_res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            ssize_t bytes = read(stdout_pipe[0], buffer.data(), buffer.size());
            if (bytes > 0) {
                result.stdout_output.append(buffer.data(), static_cast<size_t>(bytes));
            } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                stdout_open = false;
            }
        }

        if (fds[1].revents & (POLLIN | POLLHUP | POLLERR)) {
            ssize_t bytes = read(stderr_pipe[0], buffer.data(), buffer.size());
            if (bytes > 0) {
                result.stderr_output.append(buffer.data(), static_cast<size_t>(bytes));
            } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                stderr_open = false;
            }
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
        result.success = (result.exit_code == 0);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
        result.success = false;
    }

    return result;
}

#else

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    uint32_t timeout_ms,
    const std::string& working_dir
) {
    ProcessResult result;
    // Basic fallback implementation for Windows
    std::string cmd = "\"" + binary + "\"";
    for (const auto& a : args) {
        cmd += " \"" + a + "\"";
    }
    int code = std::system(cmd.c_str());
    result.exit_code = code;
    result.success = (code == 0);
    return result;
}

#endif

} // namespace nxdev::pack
