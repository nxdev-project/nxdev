#include <nxdev/cli/commands.hpp>
#include <nxdev/run/run_session.hpp>
#include <nxdev/device/device_manager.hpp>
#include <iostream>
#include <sstream>

namespace nxdev::cli {

int RunCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    run::RunOptions options;
    std::string explicit_device;
    std::string explicit_host;
    bool connectivity_check_only = false;
    bool one_shot_json = false;

    options.project_root = (ctx.project.has_value() && ctx.project->is_valid()) ? ctx.project->root_path() : "";

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--profile" && i + 1 < args.size()) {
            options.profile = args[++i];
        } else if (args[i] == "--device" && i + 1 < args.size()) {
            explicit_device = args[++i];
        } else if (args[i] == "--host" && i + 1 < args.size()) {
            explicit_host = args[++i];
        } else if (args[i] == "--no-build") {
            options.no_build = true;
        } else if (args[i] == "--no-pack") {
            options.no_pack = true;
        } else if (args[i] == "--args" && i + 1 < args.size()) {
            options.app_args = args[++i];
        } else if (args[i] == "--") {
            // All remaining args are application args
            std::ostringstream app_oss;
            for (size_t j = i + 1; j < args.size(); ++j) {
                if (j > i + 1) app_oss << " ";
                app_oss << args[j];
            }
            options.app_args = app_oss.str();
            break;
        } else if (args[i] == "--json-stream") {
            options.json_stream = true;
        } else if (args[i] == "--json") {
            one_shot_json = true;
        } else if (args[i] == "--check") {
            connectivity_check_only = true;
        } else if (args[i] == "--dry-run") {
            options.dry_run = true;
        }
    }

    device::DeviceManager dm("", options.project_root);
    std::string resolve_err;
    if (!dm.resolve_target(explicit_host, explicit_device, options.device, resolve_err)) {
        if (options.json_stream || one_shot_json) {
            std::cout << "{\"version\":\"1.0\",\"type\":\"status\",\"state\":\"failed\",\"error\":\"" << resolve_err << "\"}\n";
        } else {
            std::cerr << "Error: " << resolve_err << "\n";
        }
        return 1;
    }

    if (connectivity_check_only) {
        if (!options.json_stream && !one_shot_json) {
            std::cout << "Checking connectivity to device '" << options.device.id << "' (" << options.device.host << ")...\n";
        }
        bool ok = device::DeviceManager::test_connectivity(options.device, 3000);
        if (options.json_stream || one_shot_json) {
            std::cout << "{\"device\":\"" << options.device.id << "\",\"host\":\"" << options.device.host << "\",\"reachable\":" << (ok ? "true" : "false") << "}\n";
        } else {
            if (ok) {
                std::cout << "✔ Host " << options.device.host << " is reachable.\n";
            } else {
                std::cout << "✗ Host " << options.device.host << " is not responding.\n";
            }
        }
        return ok ? 0 : 1;
    }

    run::RunSession session(options);
    auto summary = session.execute();

    if (one_shot_json && !options.json_stream) {
        std::ostringstream oss;
        oss << "{\"success\":" << (summary.success ? "true" : "false")
            << ",\"state\":\"" << to_string(summary.final_state) << "\""
            << ",\"device\":\"" << summary.device_id << "\""
            << ",\"host\":\"" << summary.device_host << "\""
            << ",\"profile\":\"" << summary.profile << "\""
            << ",\"nro\":\"" << summary.nro_path << "\""
            << ",\"elf\":\"" << summary.elf_path << "\""
            << ",\"duration_ms\":" << summary.duration_ms
            << ",\"exit_code\":" << summary.exit_code
            << ",\"log_lines\":" << summary.total_log_lines
            << "}";
        std::cout << oss.str() << "\n";
    }

    return summary.success ? 0 : 1;
}

} // namespace nxdev::cli
