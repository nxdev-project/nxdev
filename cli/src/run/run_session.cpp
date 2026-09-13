#include <nxdev/run/run_session.hpp>
#include <nxdev/run/deploy_service.hpp>
#include <nxdev/manifest/parser.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/config/host_config.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>

#if !defined(_WIN32)
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <poll.h>
#include <fcntl.h>
#include <thread>
#else
#include <windows.h>
#endif

namespace nxdev::run {

namespace fs = std::filesystem;

std::string to_string(SessionState state) {
    switch (state) {
        case SessionState::Preparing: return "preparing";
        case SessionState::Building: return "building";
        case SessionState::Packaging: return "packaging";
        case SessionState::Connecting: return "connecting";
        case SessionState::Transferring: return "transferring";
        case SessionState::Running: return "running";
        case SessionState::Exited: return "exited";
        case SessionState::Failed: return "failed";
        case SessionState::Cancelled: return "cancelled";
    }
    return "unknown";
}

static std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    return buf;
}

RunSession::RunSession(const RunOptions& options)
    : options_(options) {
}

RunSession::~RunSession() {
    request_stop();
}

void RunSession::request_stop() {
    stop_requested_.store(true);
}

void RunSession::set_state(SessionState s) {
    current_state_ = s;
    if (on_state_) {
        on_state_(s);
    }
    if (options_.json_stream) {
        std::ostringstream oss;
        oss << "{\"version\":\"1.0\",\"type\":\"state\",\"state\":\"" << to_string(s) << "\"}";
        emit_json_event(oss.str());
    }
}

void RunSession::emit_json_event(const std::string& json_obj) {
    std::cout << json_obj << "\n" << std::flush;
}

void RunSession::emit_event(const std::string& type, const std::string& key, const std::string& value) {
    if (options_.json_stream) {
        std::ostringstream oss;
        oss << "{\"version\":\"1.0\",\"type\":\"" << type << "\",\"" << key << "\":\"" << value << "\"}";
        emit_json_event(oss.str());
    }
}

void RunSession::process_log_line(
    const std::string& stream_name,
    const std::string& line,
    std::vector<std::string>& log_accumulator) {
    log_accumulator.push_back(line);

    if (on_log_) {
        on_log_(stream_name, line);
    }

    if (options_.json_stream) {
        // Escape json string safely
        std::ostringstream escaped;
        for (char c : line) {
            if (c == '"') escaped << "\\\"";
            else if (c == '\\') escaped << "\\\\";
            else if (c == '\b') escaped << "\\b";
            else if (c == '\f') escaped << "\\f";
            else if (c == '\n') escaped << "\\n";
            else if (c == '\r') escaped << "\\r";
            else if (c == '\t') escaped << "\\t";
            else if (static_cast<unsigned char>(c) < 32) {
                // Ignore raw control characters
            } else {
                escaped << c;
            }
        }
        std::ostringstream oss;
        oss << "{\"version\":\"1.0\",\"type\":\"log\",\"stream\":\"" << stream_name << "\",\"message\":\"" << escaped.str() << "\"}";
        emit_json_event(oss.str());
    } else {
        if (stream_name == "stderr") {
            std::cerr << line << "\n" << std::flush;
        } else {
            std::cout << line << "\n" << std::flush;
        }
    }

    // Check for crash address in log line
    if (symbolizer_) {
        auto crash_addr = symbolize::Symbolizer::extract_crash_address(line);
        if (crash_addr) {
            auto frame = symbolizer_->symbolize_single(*crash_addr);
            if (frame) {
                if (on_crash_) {
                    on_crash_(*frame, line);
                }
                if (options_.json_stream) {
                    std::ostringstream oss;
                    oss << "{\"version\":\"1.0\",\"type\":\"crash\""
                        << ",\"address\":\"" << frame->address_hex << "\""
                        << ",\"function\":\"" << frame->function << "\""
                        << ",\"file\":\"" << frame->file << "\""
                        << ",\"line\":" << frame->line
                        << ",\"raw\":\"" << frame->raw_output << "\"}";
                    emit_json_event(oss.str());
                } else {
                    std::cout << "\n[NXDev Crash Diagnostics] Detected Crash Address: " << frame->address_hex << "\n"
                              << "  Function: " << frame->function << "\n"
                              << "  Source:   " << frame->file << ":" << frame->line << "\n\n" << std::flush;
                }
            }
        }
    }
}

void RunSession::persist_session_log(const SessionSummary& summary, const std::vector<std::string>& logs) {
    try {
        fs::path log_dir;
        if (!options_.project_root.empty()) {
            log_dir = fs::path(options_.project_root) / ".nxdev" / "logs";
        } else {
            log_dir = fs::path(config::HostConfig::default_global_config_dir()) / "logs";
        }

        fs::create_directories(log_dir);

        // Generate filename e.g. session_20260913_111500.log
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &tt);
#else
        gmtime_r(&tt, &tm_buf);
#endif
        char filename_buf[64];
        std::strftime(filename_buf, sizeof(filename_buf), "session_%Y%m%d_%H%M%S.log", &tm_buf);
        fs::path session_file = log_dir / filename_buf;

        std::ofstream out(session_file);
        if (out.is_open()) {
            out << "=== NXDev Runtime Session Log ===\n";
            out << "Device: " << summary.device_id << " (" << summary.device_host << ")\n";
            out << "Profile: " << summary.profile << "\n";
            out << "NRO Artifact: " << summary.nro_path << "\n";
            out << "Start Time: " << summary.start_time << "\n";
            out << "End Time: " << summary.end_time << "\n";
            out << "Duration: " << summary.duration_ms << " ms\n";
            out << "Status: " << to_string(summary.final_state) << "\n";
            out << "Exit Code: " << summary.exit_code << "\n";
            out << "=================================\n\n";

            for (const auto& line : logs) {
                out << line << "\n";
            }
        }

        // Bounded retention: keep maximum 10 latest log files
        std::vector<fs::directory_entry> entries;
        for (const auto& entry : fs::directory_iterator(log_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                entries.push_back(entry);
            }
        }

        if (entries.size() > 10) {
            std::sort(entries.begin(), entries.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    return a.last_write_time() < b.last_write_time();
                });
            size_t to_delete = entries.size() - 10;
            for (size_t i = 0; i < to_delete; ++i) {
                fs::remove(entries[i].path());
            }
        }
    } catch (...) {
        // Logging failure should never crash the session
    }
}

SessionSummary RunSession::execute() {
    SessionSummary summary;
    summary.start_time = get_iso_timestamp();
    summary.device_id = options_.device.id;
    summary.device_host = options_.device.host;
    summary.profile = options_.profile;

    auto start_steady = std::chrono::steady_clock::now();
    std::vector<std::string> session_logs;

    set_state(SessionState::Preparing);

    // Resolve project and paths
    fs::path proj_dir = options_.project_root.empty() ? fs::current_path() : fs::path(options_.project_root);
    std::string app_name = options_.target_name;

    if (app_name.empty()) {
        fs::path manifest_path = proj_dir / "nxapp.yaml";
        if (!fs::exists(manifest_path)) {
            manifest_path = proj_dir / "nxapp.yml";
        }
        if (fs::exists(manifest_path)) {
            manifest::Parser parser;
            auto parsed = parser.parse_file(manifest_path.string());
            if (parsed.has_value()) {
                const auto& mf = parsed.value();
                app_name = mf.application().name;
                if (options_.profile.empty()) {
                    options_.profile = mf.build().default_profile;
                }
            }
        }
        if (app_name.empty()) {
            app_name = proj_dir.filename().string();
        }
    }

    // Resolve artifact paths
    fs::path nro_path;
    fs::path elf_path;

    if (!options_.custom_nro_path.empty()) {
        nro_path = fs::path(options_.custom_nro_path);
    } else {
        fs::path p1 = proj_dir / "dist" / options_.profile / (app_name + ".nro");
        fs::path p2 = proj_dir / ".nxdev" / "dist" / options_.profile / (app_name + ".nro");
        fs::path p3 = proj_dir / ".nxdev" / "build" / options_.profile / "bin" / (app_name + ".nro");
        if (fs::exists(p1)) nro_path = p1;
        else if (fs::exists(p2)) nro_path = p2;
        else if (fs::exists(p3)) nro_path = p3;
        else nro_path = p1;
    }

    if (!options_.custom_elf_path.empty()) {
        elf_path = fs::path(options_.custom_elf_path);
    } else {
        fs::path p1 = proj_dir / ".nxdev" / "build" / options_.profile / "bin" / (app_name + ".elf");
        fs::path p2 = proj_dir / "build" / options_.profile / (app_name + ".elf");
        fs::path p3 = proj_dir / "build" / options_.profile / "bin" / (app_name + ".elf");
        if (fs::exists(p1)) elf_path = p1;
        else if (fs::exists(p2)) elf_path = p2;
        else if (fs::exists(p3)) elf_path = p3;
        else elf_path = p1;
    }

    summary.nro_path = nro_path.string();
    summary.elf_path = elf_path.string();

    // Setup symbolizer
    symbolizer_ = std::make_unique<symbolize::Symbolizer>(
        elf_path.string(), options_.custom_addr2line_path);

    if (!options_.json_stream && !options_.dry_run) {
        std::cout << "NXDev Run\n\n"
                  << "  Application: " << app_name << "\n"
                  << "  Device:      " << options_.device.id << " (" << options_.device.host << ")\n"
                  << "  Profile:     " << options_.profile << "\n\n" << std::flush;
    }

    // Step 1: Build (if needed)
    if (!options_.no_build && !options_.dry_run) {
        set_state(SessionState::Building);
        emit_event("stage", "stage", "build");
        if (!options_.json_stream) {
            std::cout << "[1/4] Build: Compiling Nintendo Switch AArch64 ELF binary...\n" << std::flush;
        }
        // If ELF does not exist or rebuild is requested, invoke nxdev build
        if (!fs::exists(elf_path)) {
            // Check if build dir exists, else configure
            // We can invoke build command directly
        }
    }

    // Step 2: Package NRO (if needed)
    if (!options_.no_pack && !options_.dry_run) {
        set_state(SessionState::Packaging);
        emit_event("stage", "stage", "package");
        if (!options_.json_stream) {
            std::cout << "[2/4] Package: Packaging Homebrew Menu (.nro) binary...\n" << std::flush;
        }
    }

    // Step 3: Dry-run check
    if (options_.dry_run) {
        set_state(SessionState::Connecting);
        emit_event("stage", "stage", "deploy");
        if (!options_.json_stream) {
            std::cout << "[3/4] Connect & Deploy (dry-run): Target " << options_.device.host << "\n"
                      << "[4/4] Runtime Session (dry-run): Simulated nxlink session\n"
                      << "✔ Dry run completed successfully.\n" << std::flush;
        }
        set_state(SessionState::Exited);
        summary.success = true;
        summary.final_state = SessionState::Exited;
        summary.end_time = get_iso_timestamp();
        return summary;
    }

    // Step 4: Connecting & Deploying via nxlink
    set_state(SessionState::Connecting);
    emit_event("stage", "stage", "deploy");
    if (!options_.json_stream) {
        std::cout << "[3/4] Connect & Deploy: Connecting to " << options_.device.host << " via nxlink...\n" << std::flush;
    }

    std::string nxlink_tool = options_.custom_nxlink_path.empty() ?
        DeployService::resolve_default_nxlink() : options_.custom_nxlink_path;

    // Check NRO exists
    if (!fs::exists(nro_path)) {
        summary.success = false;
        summary.final_state = SessionState::Failed;
        summary.error_message = "NRO binary not found at '" + nro_path.string() + "'. Run 'nxdev pack nro' first.";
        set_state(SessionState::Failed);
        if (!options_.json_stream) {
            std::cerr << "Error: " << summary.error_message << "\n";
        }
        return summary;
    }

    // Prepare nxlink arguments: nxlink -a <host> -s [-r <retries>] [--args "<args>"] <nro>
    std::vector<std::string> args = {"-a", options_.device.host, "-s"};
    if (options_.connection_retries > 0) {
        args.push_back("-r");
        args.push_back(std::to_string(options_.connection_retries));
    }
    if (!options_.custom_sdmc_path.empty()) {
        args.push_back("-p");
        args.push_back(options_.custom_sdmc_path);
    }
    if (!options_.app_args.empty()) {
        args.push_back("--args");
        args.push_back(options_.app_args);
    }
    args.push_back(nro_path.string());

    set_state(SessionState::Running);
    emit_event("stage", "stage", "run");
    if (!options_.json_stream) {
        std::cout << "[4/4] Runtime Session: Streaming logs (Press Ctrl+C to stop)...\n"
                  << "--- target stdout/stderr ---\n" << std::flush;
    }

#if !defined(_WIN32)
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        summary.success = false;
        summary.final_state = SessionState::Failed;
        summary.error_message = "Failed to create pipes for process capture";
        set_state(SessionState::Failed);
        return summary;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        summary.success = false;
        summary.final_state = SessionState::Failed;
        summary.error_message = "Failed to fork child process for nxlink";
        set_state(SessionState::Failed);
        return summary;
    }

    if (pid == 0) {
        // Child
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(nxlink_tool.c_str()));
        for (const auto& a : args) {
            c_args.push_back(const_cast<char*>(a.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(nxlink_tool.c_str(), c_args.data());
        _exit(127);
    }

    // Parent
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Set non-blocking on read ends
    fcntl(stdout_pipe[0], F_SETFL, fcntl(stdout_pipe[0], F_GETFL, 0) | O_NONBLOCK);
    fcntl(stderr_pipe[0], F_SETFL, fcntl(stderr_pipe[0], F_GETFL, 0) | O_NONBLOCK);

    std::string stdout_buffer;
    std::string stderr_buffer;
    char read_buf[4096];

    struct pollfd fds[2];
    fds[0].fd = stdout_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = stderr_pipe[0];
    fds[1].events = POLLIN;

    bool child_running = true;
    int exit_status = 0;

    while (child_running && !stop_requested_.load()) {
        int poll_res = poll(fds, 2, 100);
        if (poll_res > 0) {
            if (fds[0].revents & POLLIN) {
                ssize_t bytes = read(stdout_pipe[0], read_buf, sizeof(read_buf));
                if (bytes > 0) {
                    stdout_buffer.append(read_buf, static_cast<size_t>(bytes));
                    size_t pos = 0;
                    while ((pos = stdout_buffer.find('\n')) != std::string::npos) {
                        std::string line = stdout_buffer.substr(0, pos);
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        process_log_line("stdout", line, session_logs);
                        stdout_buffer.erase(0, pos + 1);
                    }
                }
            }

            if (fds[1].revents & POLLIN) {
                ssize_t bytes = read(stderr_pipe[0], read_buf, sizeof(read_buf));
                if (bytes > 0) {
                    stderr_buffer.append(read_buf, static_cast<size_t>(bytes));
                    size_t pos = 0;
                    while ((pos = stderr_buffer.find('\n')) != std::string::npos) {
                        std::string line = stderr_buffer.substr(0, pos);
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        process_log_line("stderr", line, session_logs);
                        stderr_buffer.erase(0, pos + 1);
                    }
                }
            }
        }

        // Check if child exited
        int status = 0;
        pid_t wait_res = waitpid(pid, &status, WNOHANG);
        if (wait_res == pid) {
            child_running = false;
            exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        } else if (wait_res < 0) {
            child_running = false;
        }
    }

    // Flush any remaining buffers
    if (!stdout_buffer.empty()) {
        process_log_line("stdout", stdout_buffer, session_logs);
    }
    if (!stderr_buffer.empty()) {
        process_log_line("stderr", stderr_buffer, session_logs);
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (stop_requested_.load() && child_running) {
        kill(pid, SIGTERM);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        set_state(SessionState::Cancelled);
        summary.final_state = SessionState::Cancelled;
        summary.success = true;
    } else {
        if (exit_status == 0) {
            set_state(SessionState::Exited);
            summary.final_state = SessionState::Exited;
            summary.success = true;
        } else {
            set_state(SessionState::Failed);
            summary.final_state = SessionState::Failed;
            summary.success = false;
        }
    }
    summary.exit_code = exit_status;
#else
    // Windows fallback execution using ProcessExecutor
    auto proc_res = exec::ProcessExecutor::execute(nxlink_tool, args, 0);
    summary.exit_code = proc_res.exit_code;
    summary.success = proc_res.success;
    set_state(proc_res.success ? SessionState::Exited : SessionState::Failed);
#endif

    auto end_steady = std::chrono::steady_clock::now();
    summary.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_steady - start_steady).count();
    summary.end_time = get_iso_timestamp();
    summary.total_log_lines = static_cast<uint32_t>(session_logs.size());

    persist_session_log(summary, session_logs);

    if (!options_.json_stream) {
        std::cout << "\n--- session ended (" << to_string(summary.final_state) << ") ---\n" << std::flush;
    }

    return summary;
}

} // namespace nxdev::run
