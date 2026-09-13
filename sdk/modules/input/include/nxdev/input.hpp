#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <vector>
#include <string_view>
#include <memory>

namespace nxdev::input {

enum class Button : u64 {
    None      = 0,
    A         = 1ULL << 0,
    B         = 1ULL << 1,
    X         = 1ULL << 2,
    Y         = 1ULL << 3,
    StickL    = 1ULL << 4,
    StickR    = 1ULL << 5,
    L         = 1ULL << 6,
    R         = 1ULL << 7,
    ZL        = 1ULL << 8,
    ZR        = 1ULL << 9,
    Plus      = 1ULL << 10,
    Minus     = 1ULL << 11,
    DpadLeft  = 1ULL << 12,
    DpadUp    = 1ULL << 13,
    DpadRight = 1ULL << 14,
    DpadDown  = 1ULL << 15,
    SL_Left   = 1ULL << 16,
    SR_Left   = 1ULL << 17,
    SL_Right  = 1ULL << 18,
    SR_Right  = 1ULL << 19,
    Any       = ~0ULL
};

constexpr Button operator|(Button a, Button b) noexcept {
    return static_cast<Button>(static_cast<u64>(a) | static_cast<u64>(b));
}

constexpr Button operator&(Button a, Button b) noexcept {
    return static_cast<Button>(static_cast<u64>(a) & static_cast<u64>(b));
}

constexpr Button operator^(Button a, Button b) noexcept {
    return static_cast<Button>(static_cast<u64>(a) ^ static_cast<u64>(b));
}

constexpr Button operator~(Button a) noexcept {
    return static_cast<Button>(~static_cast<u64>(a));
}

constexpr Button& operator|=(Button& a, Button b) noexcept {
    a = a | b;
    return a;
}

constexpr Button& operator&=(Button& a, Button b) noexcept {
    a = a & b;
    return a;
}

enum class Player : u8 {
    One      = 0,
    Two      = 1,
    Three    = 2,
    Four     = 3,
    Five     = 4,
    Six      = 5,
    Seven    = 6,
    Eight    = 7,
    Handheld = 8,
    Auto     = 9
};

enum class ControllerStyle : u8 {
    Unknown       = 0,
    Handheld      = 1,
    ProController = 2,
    JoyConPair    = 3,
    JoyConLeft    = 4,
    JoyConRight   = 5,
    GameCube      = 6
};

std::string_view controller_style_to_string(ControllerStyle style) noexcept;

struct Stick {
    float x{0.0f};      ///< Normalized in range [-1.0f, 1.0f]
    float y{0.0f};      ///< Normalized in range [-1.0f, 1.0f]
    s32 raw_x{0};       ///< Raw integer position [-32768, 32767]
    s32 raw_y{0};       ///< Raw integer position [-32768, 32767]

    [[nodiscard]] Stick apply_deadzone(float threshold = 0.15f) const noexcept;
};

struct TouchPoint {
    u32 id{0};
    float x{0.0f};
    float y{0.0f};
    u32 diameter_x{0};
    u32 diameter_y{0};
    u32 rotation_angle{0};
};

/**
 * @brief Configures the maximum number of players supported across the application.
 */
void configure_input(u32 max_players = 8);

/**
 * @brief Queries active touch contact points on the Nintendo Switch touch screen.
 */
[[nodiscard]] std::vector<TouchPoint> touches();

/**
 * @brief High-level Gamepad/Controller input reader with state transitions and analog stick processing.
 */
class Controller {
public:
    explicit Controller(Player player = Player::Auto);
    ~Controller();

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    Controller(Controller&& other) noexcept;
    Controller& operator=(Controller&& other) noexcept;

    /**
     * @brief Initialize the controller and configure input layouts.
     */
    Result<void> initialize();

    /**
     * @brief Update the internal controller button and analog stick state.
     */
    void update();

    /**
     * @brief Check whether the controller is currently connected.
     */
    [[nodiscard]] bool is_connected() const noexcept;

    /**
     * @brief Check whether the specified button(s) are currently held down.
     */
    [[nodiscard]] bool down(Button button) const noexcept;

    /**
     * @brief Check whether the specified button(s) were newly pressed on this frame.
     */
    [[nodiscard]] bool pressed(Button button) const noexcept;

    /**
     * @brief Check whether the specified button(s) were newly released on this frame.
     */
    [[nodiscard]] bool released(Button button) const noexcept;

    /**
     * @brief Returns the full bitmask of buttons currently held down.
     */
    [[nodiscard]] Button buttons_down() const noexcept;

    /**
     * @brief Returns the full bitmask of buttons newly pressed on this frame.
     */
    [[nodiscard]] Button buttons_pressed() const noexcept;

    /**
     * @brief Returns the full bitmask of buttons newly released on this frame.
     */
    [[nodiscard]] Button buttons_released() const noexcept;

    /**
     * @brief Returns the processed left analog stick state.
     */
    [[nodiscard]] Stick left_stick() const noexcept;

    /**
     * @brief Returns the processed right analog stick state.
     */
    [[nodiscard]] Stick right_stick() const noexcept;

    /**
     * @brief Returns the detected controller form factor / style.
     */
    [[nodiscard]] ControllerStyle style() const noexcept;

    /**
     * @brief Returns the assigned player target.
     */
    [[nodiscard]] Player player() const noexcept;

    /**
     * @brief Direct pointer to the underlying native libnx PadState when targeting Switch.
     */
    [[nodiscard]] void* native_pad() noexcept;

    // Simulation helpers for unit tests
    void simulate_state(u64 buttons_down, s32 lx = 0, s32 ly = 0, s32 rx = 0, s32 ry = 0) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Backwards-compatible InputManager wrapper.
 */
class InputManager {
public:
    InputManager() : controller_(Player::Auto) {}
    ~InputManager() = default;

    Result<void> initialize() { return controller_.initialize(); }
    void poll() { controller_.update(); }
    [[nodiscard]] bool is_down(Button button) const noexcept { return controller_.down(button); }
    [[nodiscard]] bool is_pressed(Button button) const noexcept { return controller_.pressed(button); }
    [[nodiscard]] bool is_released(Button button) const noexcept { return controller_.released(button); }
    [[nodiscard]] Stick get_left_stick() const noexcept { return controller_.left_stick(); }
    [[nodiscard]] Stick get_right_stick() const noexcept { return controller_.right_stick(); }
    void finalize() {}

private:
    Controller controller_;
};

} // namespace nxdev::input
