#include <nxdev/run/deploy_service.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/exec/process.hpp>
#include <filesystem>
#include <chrono>

namespace nxdev::run {

namespace fs = std::filesystem;

DeployService::DeployService(const std::string& nxlink_tool_path) {
    (void)nxlink_tool_path;
}

std::string DeployService::resolve_default_nxlink() {
    auto env = env::Environment::detect(config::HostConfig::load());
    if (env.switch_tools().is_found && !env.switch_tools().path.empty()) {
        fs::path p = fs::path(env.switch_tools().path) / "bin" / "nxlink";
#if defined(_WIN32)
        p += ".exe";
#endif
        if (fs::exists(p)) return p.string();
    }

    if (const char* dkp = std::getenv("DEVKITPRO")) {
        fs::path p = fs::path(dkp) / "tools" / "bin" / "nxlink";
#if defined(_WIN32)
        p += ".exe";
#endif
        if (fs::exists(p)) return p.string();
    }

    return "nxlink";
}

DeployResult DeployService::deploy(const DeployOptions& options) {
    DeployResult result;
    result.device_id = options.device.id;
    result.device_host = options.device.host;
    result.artifact_path = options.artifact_path;

    auto start_time = std::chrono::steady_clock::now();

    if (options.device.host.empty()) {
        result.success = false;
        result.error_message = "Target device host is empty.";
        return result;
    }

    if (options.dry_run) {
        result.success = true;
        result.exit_code = 0;
        result.output = "[dry-run] Would deploy '" + options.artifact_path + "' to " + options.device.host;
        result.duration_ms = 1;
        return result;
    }

    if (options.artifact_path.empty() || !fs::exists(options.artifact_path)) {
        result.success = false;
        result.error_message = "NRO artifact not found: '" + options.artifact_path + "'";
        return result;
    }

    std::string tool = options.nxlink_tool_path.empty() ? resolve_default_nxlink() : options.nxlink_tool_path;

    // Build arguments: nxlink -a <host> [-p <sdmc-path>] [-r <retries>] <artifact>
    std::vector<std::string> args = {"-a", options.device.host};
    if (options.retries > 0) {
        args.push_back("-r");
        args.push_back(std::to_string(options.retries));
    }
    if (!options.custom_sdmc_path.empty()) {
        args.push_back("-p");
        args.push_back(options.custom_sdmc_path);
    }
    args.push_back(options.artifact_path);

    auto proc_res = exec::ProcessExecutor::execute(tool, args, 30000);
    auto end_time = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    result.exit_code = proc_res.exit_code;
    result.output = proc_res.combined_output();

    if (proc_res.success) {
        result.success = true;
    } else {
        result.success = false;
        if (proc_res.timed_out) {
            result.error_message = "Deployment timed out connecting to " + options.device.host;
        } else if (!proc_res.stderr_output.empty()) {
            result.error_message = proc_res.stderr_output;
        } else {
            result.error_message = "nxlink exited with code " + std::to_string(proc_res.exit_code);
        }
    }

    return result;
}

} // namespace nxdev::run
