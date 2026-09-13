#pragma once

#include <nxdev/device/device.hpp>
#include <vector>
#include <optional>
#include <string>
#include <memory>

namespace nxdev::device {

/**
 * @brief Manages configured devices, storage persistence, and device resolution.
 */
class DeviceManager {
public:
    explicit DeviceManager(
        const std::string& custom_config_path = "",
        const std::string& project_root = "");
    ~DeviceManager() = default;

    /**
     * @brief Reloads devices from disk.
     */
    void reload();

    /**
     * @brief Saves devices to disk.
     */
    [[nodiscard]] bool save();

    /**
     * @brief Lists all configured devices.
     */
    [[nodiscard]] std::vector<Device> list_devices() const;

    /**
     * @brief Retrieves a device by its unique ID.
     */
    [[nodiscard]] std::optional<Device> get_device(const std::string& id) const;

    /**
     * @brief Finds a device by ID, Name, or Host.
     */
    [[nodiscard]] std::optional<Device> find_device(const std::string& query) const;

    /**
     * @brief Gets the currently configured default device, if any.
     */
    [[nodiscard]] std::optional<Device> get_default_device() const;

    /**
     * @brief Adds or updates a device.
     */
    bool add_device(const Device& device, bool set_as_default = false);

    /**
     * @brief Removes a device by its ID.
     */
    bool remove_device(const std::string& id);

    /**
     * @brief Sets the specified device as the default target.
     */
    bool set_default_device(const std::string& id);

    /**
     * @brief Resolves target device according to NXDev precedence rules:
     * 1. Explicit CLI --host
     * 2. Explicit CLI --device
     * 3. Project-local default device
     * 4. Global default device
     * 5. Single configured device fallback
     * 
     * @return pair of <success, Device> or error message in error_msg out-param
     */
    [[nodiscard]] bool resolve_target(
        const std::string& explicit_host,
        const std::string& explicit_device,
        Device& out_device,
        std::string& out_error_message) const;

    /**
     * @brief Tests basic network reachability for the given device.
     */
    [[nodiscard]] static bool test_connectivity(const Device& device, int timeout_ms = 3000);

    [[nodiscard]] const std::string& config_path() const noexcept { return config_path_; }

private:
    std::string config_path_;
    std::string project_root_;
    std::vector<Device> devices_;

    void load_from_file(const std::string& path);
};

} // namespace nxdev::device
