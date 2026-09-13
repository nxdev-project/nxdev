#pragma once

#include <nxdev/device/device.hpp>
#include <nxdev/symbolize/symbolizer.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <cstdint>

namespace nxdev::run {

enum class SessionState {
    Preparing,
    Building,
    Packaging,
    Connecting,
    Transferring,
    Running,
    Exited,
    Failed,
    Cancelled
};

[[nodiscard]] std::string to_string(SessionState state);

struct RunOptions {
    std::string project_root;
    device::Device device;
    std::string profile{"debug"};
    std::string target_name;
    bool no_build{false};
    bool no_pack{false};
    std::string app_args;
    bool json_stream{false};
    bool verbose{false};
    bool dry_run{false};
    std::string custom_nxlink_path;
    std::string custom_addr2line_path;
    std::string custom_elf_path;
    std::string custom_nro_path;
    std::string custom_sdmc_path;
    uint32_t connection_retries{3};
};

struct SessionSummary {
    bool success{false};
    SessionState final_state{SessionState::Preparing};
    int exit_code{0};
    std::string device_id;
    std::string device_host;
    std::string profile;
    std::string elf_path;
    std::string nro_path;
    std::string start_time;
    std::string end_time;
    uint64_t duration_ms{0};
    uint32_t total_log_lines{0};
    uint32_t crash_count{0};
    std::string session_log_path;
    std::string error_message;
};

/**
 * @brief Manages a live development session on Nintendo Switch via nxlink.
 */
class RunSession {
public:
    explicit RunSession(const RunOptions& options);
    ~RunSession();

    /**
     * @brief Executes the complete run pipeline (build -> pack -> connect -> live stream).
     */
    [[nodiscard]] SessionSummary execute();

    /**
     * @brief Requests clean cancellation of the active run session.
     */
    void request_stop();

    [[nodiscard]] bool is_cancelled() const noexcept { return stop_requested_.load(); }
    [[nodiscard]] SessionState state() const noexcept { return current_state_; }

    // Stream callback types
    using LogCallback = std::function<void(const std::string& stream, const std::string& line)>;
    using StateCallback = std::function<void(SessionState new_state)>;
    using CrashCallback = std::function<void(const symbolize::SymbolFrame& frame, const std::string& raw)>;

    void set_on_log(LogCallback cb) { on_log_ = std::move(cb); }
    void set_on_state(StateCallback cb) { on_state_ = std::move(cb); }
    void set_on_crash(CrashCallback cb) { on_crash_ = std::move(cb); }

private:
    RunOptions options_;
    std::atomic<bool> stop_requested_{false};
    SessionState current_state_{SessionState::Preparing};
    std::unique_ptr<symbolize::Symbolizer> symbolizer_;

    LogCallback on_log_;
    StateCallback on_state_;
    CrashCallback on_crash_;

    void set_state(SessionState s);
    void emit_event(const std::string& type, const std::string& key, const std::string& value);
    void emit_json_event(const std::string& json_obj);
    void process_log_line(const std::string& stream_name, const std::string& line, std::vector<std::string>& log_accumulator);
    void persist_session_log(const SessionSummary& summary, const std::vector<std::string>& logs);
};

} // namespace nxdev::run
