#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace nxdev::device {

/**
 * @brief Represents a target development device (e.g. Nintendo Switch running Homebrew Menu / nxlink).
 */
struct Device {
    std::string id;              // Unique slug identifier, e.g. "dev-switch"
    std::string name;            // User-friendly display name
    std::string host;            // IPv4 address, IPv6 address, or hostname
    std::string transport{"nxlink"}; // Transport protocol (default: "nxlink")
    uint16_t port{0};            // Port (0 for protocol default)
    bool is_default{false};      // Whether this is the default target device
    std::string created_at;      // ISO 8601 timestamp or date string

    /**
     * @brief Validates device ID syntax (alphanumeric, dashes, underscores, 1-64 chars).
     */
    [[nodiscard]] static bool is_valid_id(const std::string& id) noexcept;

    /**
     * @brief Validates host syntax (IPv4, IPv6, or valid hostname).
     */
    [[nodiscard]] static bool is_valid_host(const std::string& host) noexcept;

    /**
     * @brief Formats device description for CLI output.
     */
    [[nodiscard]] std::string display_summary() const;
};

} // namespace nxdev::device
