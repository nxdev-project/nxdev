#include <nxdev/display.hpp>

namespace nxdev::display {

Result<void> DisplayManager::initialize() {
    // Switch vi/applet display query wrapper in future stages
    return Result<void>::Success();
}

Resolution DisplayManager::get_resolution() const noexcept {
    return current_resolution_;
}

ConsoleMode DisplayManager::get_console_mode() const noexcept {
    return mode_;
}

void DisplayManager::finalize() {
    // Finalize display subsystems
}

} // namespace nxdev::display
