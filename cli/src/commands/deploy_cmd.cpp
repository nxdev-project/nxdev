#include <nxdev/cli/commands.hpp>
#include <nxdev/run/deploy_service.hpp>
#include <nxdev/device/device_manager.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace nxdev::cli {

namespace fs = std::filesystem;

int DeployCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    run::DeployOptions options;
    std::string explicit_device;
    std::string explicit_host;
    std::string custom_artifact;
    std::string profile = "debug";
    bool json_output = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--device" && i + 1 < args.size()) {
            explicit_device = args[++i];
        } else if (args[i] == "--host" && i + 1 < args.size()) {
            explicit_host = args[++i];
        } else if ((args[i] == "--artifact" || args[i] == "-a") && i + 1 < args.size()) {
            custom_artifact = args[++i];
        } else if (args[i] == "--profile" && i + 1 < args.size()) {
            profile = args[++i];
        } else if (args[i] == "--path" && i + 1 < args.size()) {
            options.custom_sdmc_path = args[++i];
        } else if (args[i] == "--dry-run") {
            options.dry_run = true;
        } else if (args[i] == "--json") {
            json_output = true;
        }
    }

    std::string proj_root = (ctx.project.has_value() && ctx.project->is_valid()) ? ctx.project->root_path() : "";
    device::DeviceManager dm("", proj_root);

    std::string resolve_err;
    if (!dm.resolve_target(explicit_host, explicit_device, options.device, resolve_err)) {
        if (json_output) {
            std::cout << "{\"success\":false,\"error\":\"" << resolve_err << "\"}\n";
        } else {
            std::cerr << "Error: " << resolve_err << "\n";
        }
        return 1;
    }

    // Resolve artifact path
    if (!custom_artifact.empty()) {
        options.artifact_path = custom_artifact;
    } else if (ctx.project.has_value() && ctx.project->is_valid()) {
        std::string app_name = ctx.project->manifest().application().name;
        fs::path candidate = fs::path(proj_root) / "dist" / profile / (app_name + ".nro");
        options.artifact_path = candidate.string();
    } else {
        std::string err = "No artifact specified and no active project found. Pass --artifact <path.nro>.";
        if (json_output) {
            std::cout << "{\"success\":false,\"error\":\"" << err << "\"}\n";
        } else {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }

    if (!options.dry_run && !fs::exists(options.artifact_path)) {
        std::string err = "Artifact not found at '" + options.artifact_path + "'. Build and package the project first with 'nxdev pack nro'.";
        if (json_output) {
            std::cout << "{\"success\":false,\"error\":\"" << err << "\"}\n";
        } else {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }

    if (!json_output) {
        std::cout << "Deploying " << options.artifact_path << " to " << options.device.host << "...\n";
    }

    run::DeployService svc;
    auto result = svc.deploy(options);

    if (json_output) {
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false")
            << ",\"device\":\"" << result.device_id << "\""
            << ",\"host\":\"" << result.device_host << "\""
            << ",\"artifact\":\"" << result.artifact_path << "\""
            << ",\"duration_ms\":" << result.duration_ms
            << ",\"exit_code\":" << result.exit_code;
        if (!result.error_message.empty()) {
            oss << ",\"error\":\"" << result.error_message << "\"";
        }
        oss << "}";
        std::cout << oss.str() << "\n";
    } else {
        if (result.success) {
            std::cout << "✔ Deployed successfully to " << options.device.host << " (" << result.duration_ms << "ms)\n";
        } else {
            std::cerr << "✗ Deployment failed: " << result.error_message << "\n";
        }
    }

    return result.success ? 0 : 1;
}

} // namespace nxdev::cli
