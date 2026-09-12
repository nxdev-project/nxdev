#include <nxdev/input.hpp>

namespace nxdev::input {

Result InputManager::initialize() {
    // Pad / hid service initialization (libnx wrapper) will be wired when targeting Switch
    keys_held_ = 0;
    keys_pressed_ = 0;
    keys_released_ = 0;
    return results::Success;
}

void InputManager::poll() {
    // Polling logic for Switch hidScanInput / padUpdate in future stages
}

bool InputManager::is_down(Button button) const noexcept {
    return (keys_held_ & static_cast<u64>(button)) != 0;
}

bool InputManager::is_pressed(Button button) const noexcept {
    return (keys_pressed_ & static_cast<u64>(button)) != 0;
}

bool InputManager::is_released(Button button) const noexcept {
    return (keys_released_ & static_cast<u64>(button)) != 0;
}

AnalogStick InputManager::get_left_stick() const noexcept {
    return left_stick_;
}

AnalogStick InputManager::get_right_stick() const noexcept {
    return right_stick_;
}

void InputManager::finalize() {
    keys_held_ = 0;
    keys_pressed_ = 0;
    keys_released_ = 0;
}

} // namespace nxdev::input
