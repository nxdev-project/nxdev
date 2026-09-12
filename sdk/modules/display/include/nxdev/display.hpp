#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>

namespace nxdev::display {

struct Resolution {
    u32 width{1280};
    u32 height{720};
};

enum class ConsoleMode {
    Handheld,
    Docked
};

class DisplayManager {
public:
    DisplayManager() = default;
    ~DisplayManager() = default;

    Result initialize();
    [[nodiscard]] Resolution get_resolution() const noexcept;
    [[nodiscard]] ConsoleMode get_console_mode() const noexcept;
    void finalize();

private:
    Resolution current_resolution_{1280, 720};
    ConsoleMode mode_{ConsoleMode::Handheld};
};

} // namespace nxdev::display
