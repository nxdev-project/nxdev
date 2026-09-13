#pragma once

#include <nxdev/types.hpp>
#include <nxdev/app.hpp>
#include <string>

namespace nxdev::system {

struct FirmwareVersion {
    u8 major{0};
    u8 minor{0};
    u8 micro{0};
    std::string display_version;
};

/**
 * @brief Returns the active applet execution type under Horizon OS.
 */
[[nodiscard]] AppletType get_applet_type() noexcept;

/**
 * @brief Returns the Nintendo Switch system firmware version.
 */
[[nodiscard]] FirmwareVersion get_firmware_version() noexcept;

/**
 * @brief Returns the total available system memory in bytes.
 */
[[nodiscard]] u64 get_total_memory() noexcept;

/**
 * @brief Returns the current used system memory in bytes.
 */
[[nodiscard]] u64 get_used_memory() noexcept;

} // namespace nxdev::system
