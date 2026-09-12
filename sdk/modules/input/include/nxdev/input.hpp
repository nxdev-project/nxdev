#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>

namespace nxdev::input {

enum class Button : u64 {
    A = 1ULL << 0,
    B = 1ULL << 1,
    X = 1ULL << 2,
    Y = 1ULL << 3,
    LStick = 1ULL << 4,
    RStick = 1ULL << 5,
    L = 1ULL << 6,
    R = 1ULL << 7,
    ZL = 1ULL << 8,
    ZR = 1ULL << 9,
    Plus = 1ULL << 10,
    Minus = 1ULL << 11,
    DpadLeft = 1ULL << 12,
    DpadUp = 1ULL << 13,
    DpadRight = 1ULL << 14,
    DpadDown = 1ULL << 15
};

struct AnalogStick {
    s32 x{0};
    s32 y{0};
};

class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    Result initialize();
    void poll();
    [[nodiscard]] bool is_down(Button button) const noexcept;
    [[nodiscard]] bool is_pressed(Button button) const noexcept;
    [[nodiscard]] bool is_released(Button button) const noexcept;
    [[nodiscard]] AnalogStick get_left_stick() const noexcept;
    [[nodiscard]] AnalogStick get_right_stick() const noexcept;
    void finalize();

private:
    u64 keys_held_{0};
    u64 keys_pressed_{0};
    u64 keys_released_{0};
    AnalogStick left_stick_{};
    AnalogStick right_stick_{};
};

} // namespace nxdev::input
