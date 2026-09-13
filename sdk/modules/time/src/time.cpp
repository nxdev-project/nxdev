#include <nxdev/time.hpp>
#include <iomanip>
#include <sstream>
#include <ctime>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::time {

u64 get_system_tick() noexcept {
#ifdef __SWITCH__
    return armGetSystemTick();
#else
    auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
    return nanoseconds_to_ticks(static_cast<u64>(ns));
#endif
}

u64 get_system_tick_frequency() noexcept {
#ifdef __SWITCH__
    return armGetSystemTickFreq();
#else
    return 19200000ULL; // 19.2 MHz standard Switch timer freq
#endif
}

std::chrono::nanoseconds monotonic_time() noexcept {
#ifdef __SWITCH__
    u64 tick = armGetSystemTick();
    return std::chrono::nanoseconds(armTicksToNs(tick));
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    );
#endif
}

std::chrono::milliseconds monotonic_time_ms() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(monotonic_time());
}

Clock::time_point Clock::now() noexcept {
    return Clock::time_point(monotonic_time());
}

std::string DateTime::format_iso8601() const {
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << year << "-"
       << std::setw(2) << static_cast<int>(month) << "-"
       << std::setw(2) << static_cast<int>(day) << "T"
       << std::setw(2) << static_cast<int>(hour) << ":"
       << std::setw(2) << static_cast<int>(minute) << ":"
       << std::setw(2) << static_cast<int>(second) << "Z";
    return ss.str();
}

DateTime wall_clock_now() noexcept {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};

#if defined(_WIN32)
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif

    auto duration_since_sec = now.time_since_epoch() - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_sec).count();

    DateTime dt;
    dt.year = static_cast<u16>(1900 + tm_buf.tm_year);
    dt.month = static_cast<u8>(1 + tm_buf.tm_mon);
    dt.day = static_cast<u8>(tm_buf.tm_mday);
    dt.hour = static_cast<u8>(tm_buf.tm_hour);
    dt.minute = static_cast<u8>(tm_buf.tm_min);
    dt.second = static_cast<u8>(tm_buf.tm_sec);
    dt.millisecond = static_cast<u32>(ms >= 0 ? ms : 0);
    return dt;
}

} // namespace nxdev::time
