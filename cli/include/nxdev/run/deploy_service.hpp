#pragma once

#include <nxdev/device/device.hpp>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <cstdint>

namespace nxdev::run {

struct DeployOptions {
    device::Device device;
    std::string artifact_path;   // Path to .nro file
    std::string custom_sdmc_path; // e.g. "sdmc:/switch/myapp.nro"
    int retries{3};
    bool dry_run{false};
    bool verbose{false};
    std::string nxlink_tool_path; // Custom tool path or auto-detected
};

struct DeployResult {
    bool success{false};
    int exit_code{-1};
    std::string device_id;
    std::string device_host;
    std::string artifact_path;
    std::string error_message;
    std::string output;
    uint64_t duration_ms{0};
};

class DeployService {
public:
    explicit DeployService(const std::string& nxlink_tool_path = "");
    ~DeployService() = default;

    /**
     * @brief Resolves default nxlink tool from environment or PATH.
     */
    [[nodiscard]] static std::string resolve_default_nxlink();

    /**
     * @brief Deploys an NRO artifact to the target device using nxlink.
     */
    [[nodiscard]] DeployResult deploy(const DeployOptions& options);
};

} // namespace nxdev::run
