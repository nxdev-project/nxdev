#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <string_view>
#include <functional>
#include <memory>

namespace nxdev::power {

namespace results {
    inline constexpr ResultCode ServiceUnavailable{0x00050001};
}

enum class OperationMode : u8 {
    Handheld = 0,
    Docked   = 1,
    Unknown  = 2
};

std::string_view operation_mode_to_string(OperationMode mode) noexcept;

enum class FocusState : u8 {
    InFocus    = 0,
    OutOfFocus = 1,
    Background = 2
};

std::string_view focus_state_to_string(FocusState state) noexcept;

enum class PerformanceMode : u8 {
    Normal = 0,
    Boost  = 1
};

std::string_view performance_mode_to_string(PerformanceMode mode) noexcept;

enum class AppletEvent : u8 {
    FocusChanged,
    OperationModeChanged,
    PerformanceModeChanged,
    ExitRequested,
    Resumed
};

struct BatteryInfo {
    u32 percentage{100};
    bool is_charging{false};
};

/**
 * @brief Returns the console's current physical operation mode (Handheld or Docked).
 */
[[nodiscard]] OperationMode get_operation_mode() noexcept;

/**
 * @brief Returns whether the application currently has window/user input focus.
 */
[[nodiscard]] FocusState get_focus_state() noexcept;

/**
 * @brief Returns the active APM performance profile (Normal or Boost).
 */
[[nodiscard]] PerformanceMode get_performance_mode() noexcept;

/**
 * @brief Queries current battery charge level and charging status.
 */
[[nodiscard]] Result<BatteryInfo> get_battery_info() noexcept;

/**
 * @brief RAII Registration guard for Horizon OS applet status hooks.
 */
class EventHookRegistration {
public:
    EventHookRegistration() noexcept = default;
    ~EventHookRegistration();

    EventHookRegistration(const EventHookRegistration&) = delete;
    EventHookRegistration& operator=(const EventHookRegistration&) = delete;

    EventHookRegistration(EventHookRegistration&& other) noexcept;
    EventHookRegistration& operator=(EventHookRegistration&& other) noexcept;

    [[nodiscard]] bool is_active() const noexcept {
        return active_;
    }

    explicit operator bool() const noexcept {
        return active_;
    }

    struct Impl;
    void unregister() noexcept;

private:
    friend EventHookRegistration register_event_handler(std::function<void(AppletEvent)> handler);

    std::unique_ptr<Impl> impl_;
    bool active_{false};
};

/**
 * @brief Registers an RAII callback invoked on Horizon OS applet events (focus change, dock/undock, etc.).
 */
[[nodiscard]] EventHookRegistration register_event_handler(std::function<void(AppletEvent)> handler);

} // namespace nxdev::power
