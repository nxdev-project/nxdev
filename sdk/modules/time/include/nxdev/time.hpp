#pragma once

#include <nxdev/types.hpp>
#include <chrono>
#include <string>
#include <thread>

namespace nxdev::time {

/**
 * @brief Steady high-resolution monotonic clock compatible with std::chrono.
 */
class Clock {
public:
    using duration   = std::chrono::nanoseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<Clock, duration>;

    static constexpr bool is_steady = true;

    [[nodiscard]] static time_point now() noexcept;
};

/**
 * @brief Returns elapsed monotonic time since system boot in nanoseconds.
 */
[[nodiscard]] std::chrono::nanoseconds monotonic_time() noexcept;

/**
 * @brief Returns elapsed monotonic time since system boot in milliseconds.
 */
[[nodiscard]] std::chrono::milliseconds monotonic_time_ms() noexcept;

/**
 * @brief Retrieves the raw AArch64 hardware system counter tick.
 */
[[nodiscard]] u64 get_system_tick() noexcept;

/**
 * @brief Retrieves the system counter frequency in Hz (typically 19.2 MHz on Nintendo Switch).
 */
[[nodiscard]] u64 get_system_tick_frequency() noexcept;

/**
 * @brief Converts hardware system ticks to nanoseconds.
 */
[[nodiscard]] constexpr u64 ticks_to_nanoseconds(u64 ticks) noexcept {
    return (ticks * 625) / 12;
}

/**
 * @brief Converts nanoseconds to hardware system ticks.
 */
[[nodiscard]] constexpr u64 nanoseconds_to_ticks(u64 ns) noexcept {
    return (ns * 12) / 625;
}

/**
 * @brief Structured Gregorian calendar date and time.
 */
struct DateTime {
    u16 year{1970};
    u8 month{1};
    u8 day{1};
    u8 hour{0};
    u8 minute{0};
    u8 second{0};
    u32 millisecond{0};

    [[nodiscard]] std::string format_iso8601() const;
};

/**
 * @brief Returns current real-world wall-clock calendar time (UTC).
 */
[[nodiscard]] DateTime wall_clock_now() noexcept;

/**
 * @brief Suspends execution of the current thread for the specified duration.
 */
template <typename Rep, typename Period>
inline void sleep_for(const std::chrono::duration<Rep, Period>& duration) {
    std::this_thread::sleep_for(duration);
}

} // namespace nxdev::time
