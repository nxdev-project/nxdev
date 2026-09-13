#include <nxdev/input.hpp>
#include <cmath>
#include <algorithm>
#include <mutex>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::input {

std::string_view controller_style_to_string(ControllerStyle style) noexcept {
    switch (style) {
        case ControllerStyle::Handheld: return "Handheld";
        case ControllerStyle::ProController: return "ProController";
        case ControllerStyle::JoyConPair: return "JoyConPair";
        case ControllerStyle::JoyConLeft: return "JoyConLeft";
        case ControllerStyle::JoyConRight: return "JoyConRight";
        case ControllerStyle::GameCube: return "GameCube";
        case ControllerStyle::Unknown:
        default:
            return "Unknown";
    }
}

Stick Stick::apply_deadzone(float threshold) const noexcept {
    float len = std::sqrt(x * x + y * y);
    if (len <= threshold || threshold >= 1.0f) {
        return Stick{0.0f, 0.0f, 0, 0};
    }
    float scaled = (len - threshold) / (1.0f - threshold);
    float norm_x = std::clamp((x / len) * scaled, -1.0f, 1.0f);
    float norm_y = std::clamp((y / len) * scaled, -1.0f, 1.0f);
    return Stick{
        norm_x,
        norm_y,
        static_cast<s32>(norm_x * 32767.0f),
        static_cast<s32>(norm_y * 32767.0f)
    };
}

void configure_input(u32 max_players) {
#ifdef __SWITCH__
    padConfigureInput(max_players, HidNpadStyleSet_NpadStandard);
#else
    (void)max_players;
#endif
}

std::vector<TouchPoint> touches() {
    std::vector<TouchPoint> result;
#ifdef __SWITCH__
    static std::once_flag touch_init;
    std::call_once(touch_init, []() {
        hidInitializeTouchScreen();
    });

    HidTouchScreenState state{};
    if (hidGetTouchScreenStates(&state, 1) > 0 && state.count > 0) {
        s32 count = std::clamp(state.count, 0, 16);
        result.reserve(count);
        for (s32 i = 0; i < count; ++i) {
            const auto& t = state.touches[i];
            result.push_back(TouchPoint{
                .id = t.finger_id,
                .x = static_cast<float>(t.x),
                .y = static_cast<float>(t.y),
                .diameter_x = t.diameter_x,
                .diameter_y = t.diameter_y,
                .rotation_angle = t.rotation_angle
            });
        }
    }
#endif
    return result;
}

struct Controller::Impl {
    Player player{Player::Auto};
    bool connected{false};
    u64 cur_buttons{0};
    u64 old_buttons{0};
    u64 next_buttons{0};
    Stick left_stick{};
    Stick right_stick{};
    ControllerStyle style{ControllerStyle::Unknown};

#ifdef __SWITCH__
    PadState pad{};
#endif
};

Controller::Controller(Player player) : impl_(std::make_unique<Impl>()) {
    impl_->player = player;
    (void)initialize();
}

Controller::~Controller() = default;
Controller::Controller(Controller&& other) noexcept = default;
Controller& Controller::operator=(Controller&& other) noexcept = default;

Result<void> Controller::initialize() {
#ifdef __SWITCH__
    configure_input(8);

    switch (impl_->player) {
        case Player::Handheld:
            padInitialize(&impl_->pad, HidNpadIdType_Handheld);
            break;
        case Player::Auto:
            padInitializeDefault(&impl_->pad);
            break;
        default: {
            u8 p_idx = static_cast<u8>(impl_->player);
            if (p_idx <= static_cast<u8>(Player::Eight)) {
                padInitializeWithMask(&impl_->pad, 1ULL << p_idx);
            } else {
                padInitializeDefault(&impl_->pad);
            }
            break;
        }
    }
#else
    impl_->connected = true;
    impl_->style = ControllerStyle::ProController;
#endif
    return Result<void>::Success();
}

void Controller::update() {
#ifdef __SWITCH__
    padUpdate(&impl_->pad);
    impl_->connected = padIsConnected(&impl_->pad);

    impl_->old_buttons = impl_->cur_buttons;
    impl_->cur_buttons = padGetButtons(&impl_->pad);

    // Read Left Stick
    auto raw_l = padGetStickPos(&impl_->pad, 0);
    float lx = std::clamp(static_cast<float>(raw_l.x) / 32767.0f, -1.0f, 1.0f);
    float ly = std::clamp(static_cast<float>(raw_l.y) / 32767.0f, -1.0f, 1.0f);
    impl_->left_stick = Stick{lx, ly, raw_l.x, raw_l.y};

    // Read Right Stick
    auto raw_r = padGetStickPos(&impl_->pad, 1);
    float rx = std::clamp(static_cast<float>(raw_r.x) / 32767.0f, -1.0f, 1.0f);
    float ry = std::clamp(static_cast<float>(raw_r.y) / 32767.0f, -1.0f, 1.0f);
    impl_->right_stick = Stick{rx, ry, raw_r.x, raw_r.y};

    // Determine style
    u32 styles = padGetStyleSet(&impl_->pad);
    if (styles & HidNpadStyleTag_NpadHandheld) {
        impl_->style = ControllerStyle::Handheld;
    } else if (styles & HidNpadStyleTag_NpadFullKey) {
        impl_->style = ControllerStyle::ProController;
    } else if (styles & HidNpadStyleTag_NpadJoyDual) {
        impl_->style = ControllerStyle::JoyConPair;
    } else if (styles & HidNpadStyleTag_NpadJoyLeft) {
        impl_->style = ControllerStyle::JoyConLeft;
    } else if (styles & HidNpadStyleTag_NpadJoyRight) {
        impl_->style = ControllerStyle::JoyConRight;
    } else if (styles & HidNpadStyleTag_NpadGc) {
        impl_->style = ControllerStyle::GameCube;
    } else {
        impl_->style = ControllerStyle::Unknown;
    }
#else
    impl_->old_buttons = impl_->cur_buttons;
    impl_->cur_buttons = impl_->next_buttons;
#endif
}

bool Controller::is_connected() const noexcept {
    return impl_ ? impl_->connected : false;
}

bool Controller::down(Button button) const noexcept {
    if (!impl_) return false;
    return (impl_->cur_buttons & static_cast<u64>(button)) != 0;
}

bool Controller::pressed(Button button) const noexcept {
    if (!impl_) return false;
    u64 new_down = ~impl_->old_buttons & impl_->cur_buttons;
    return (new_down & static_cast<u64>(button)) != 0;
}

bool Controller::released(Button button) const noexcept {
    if (!impl_) return false;
    u64 new_up = impl_->old_buttons & ~impl_->cur_buttons;
    return (new_up & static_cast<u64>(button)) != 0;
}

Button Controller::buttons_down() const noexcept {
    return impl_ ? static_cast<Button>(impl_->cur_buttons) : Button::None;
}

Button Controller::buttons_pressed() const noexcept {
    if (!impl_) return Button::None;
    return static_cast<Button>(~impl_->old_buttons & impl_->cur_buttons);
}

Button Controller::buttons_released() const noexcept {
    if (!impl_) return Button::None;
    return static_cast<Button>(impl_->old_buttons & ~impl_->cur_buttons);
}

Stick Controller::left_stick() const noexcept {
    return impl_ ? impl_->left_stick : Stick{};
}

Stick Controller::right_stick() const noexcept {
    return impl_ ? impl_->right_stick : Stick{};
}

ControllerStyle Controller::style() const noexcept {
    return impl_ ? impl_->style : ControllerStyle::Unknown;
}

Player Controller::player() const noexcept {
    return impl_ ? impl_->player : Player::Auto;
}

void* Controller::native_pad() noexcept {
#ifdef __SWITCH__
    return impl_ ? &impl_->pad : nullptr;
#else
    return nullptr;
#endif
}

void Controller::simulate_state(u64 buttons_down, s32 lx, s32 ly, s32 rx, s32 ry) noexcept {
    if (!impl_) return;
    impl_->connected = true;
    impl_->next_buttons = buttons_down;

    float flx = std::clamp(static_cast<float>(lx) / 32767.0f, -1.0f, 1.0f);
    float fly = std::clamp(static_cast<float>(ly) / 32767.0f, -1.0f, 1.0f);
    impl_->left_stick = Stick{flx, fly, lx, ly};

    float frx = std::clamp(static_cast<float>(rx) / 32767.0f, -1.0f, 1.0f);
    float fry = std::clamp(static_cast<float>(ry) / 32767.0f, -1.0f, 1.0f);
    impl_->right_stick = Stick{frx, fry, rx, ry};
}

} // namespace nxdev::input
