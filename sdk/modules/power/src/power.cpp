#include <nxdev/power.hpp>
#include <mutex>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::power {

std::string_view operation_mode_to_string(OperationMode mode) noexcept {
    switch (mode) {
        case OperationMode::Handheld: return "Handheld";
        case OperationMode::Docked:   return "Docked";
        case OperationMode::Unknown:
        default:
            return "Unknown";
    }
}

std::string_view focus_state_to_string(FocusState state) noexcept {
    switch (state) {
        case FocusState::InFocus:    return "InFocus";
        case FocusState::OutOfFocus: return "OutOfFocus";
        case FocusState::Background: return "Background";
        default:                     return "Unknown";
    }
}

std::string_view performance_mode_to_string(PerformanceMode mode) noexcept {
    switch (mode) {
        case PerformanceMode::Normal: return "Normal";
        case PerformanceMode::Boost:  return "Boost";
        default:                      return "Unknown";
    }
}

OperationMode get_operation_mode() noexcept {
#ifdef __SWITCH__
    ::AppletOperationMode mode = appletGetOperationMode();
    switch (mode) {
        case AppletOperationMode_Handheld: return OperationMode::Handheld;
        case AppletOperationMode_Console:  return OperationMode::Docked;
        default:                           return OperationMode::Unknown;
    }
#else
    return OperationMode::Handheld;
#endif
}

FocusState get_focus_state() noexcept {
#ifdef __SWITCH__
    ::AppletFocusState state = appletGetFocusState();
    switch (state) {
        case AppletFocusState_InFocus:    return FocusState::InFocus;
        case AppletFocusState_OutOfFocus: return FocusState::OutOfFocus;
        case AppletFocusState_Background: return FocusState::Background;
        default:                          return FocusState::InFocus;
    }
#else
    return FocusState::InFocus;
#endif
}

PerformanceMode get_performance_mode() noexcept {
#ifdef __SWITCH__
    ::ApmPerformanceMode mode = appletGetPerformanceMode();
    switch (mode) {
        case ApmPerformanceMode_Boost: return PerformanceMode::Boost;
        case ApmPerformanceMode_Normal:
        default:
            return PerformanceMode::Normal;
    }
#else
    return PerformanceMode::Normal;
#endif
}

Result<BatteryInfo> get_battery_info() noexcept {
#ifdef __SWITCH__
    ::Result rc = psmInitialize();
    if (R_FAILED(rc)) {
        return Result<BatteryInfo>(results::ServiceUnavailable);
    }

    u32 pct = 100;
    bool charging = false;
    psmGetBatteryChargePercentage(&pct);
    psmIsBatteryChargingEnabled(&charging);
    psmExit();

    return Result<BatteryInfo>(BatteryInfo{
        .percentage = pct,
        .is_charging = charging
    });
#else
    return Result<BatteryInfo>(BatteryInfo{
        .percentage = 100,
        .is_charging = false
    });
#endif
}

struct EventHookRegistration::Impl {
    std::function<void(AppletEvent)> handler;
#ifdef __SWITCH__
    AppletHookCookie cookie{};
#endif
};

#ifdef __SWITCH__
static void applet_hook_trampoline(AppletHookType hook, void* param) {
    auto* impl = static_cast<EventHookRegistration::Impl*>(param);
    if (!impl || !impl->handler) return;

    switch (hook) {
        case AppletHookType_OnFocusState:
            impl->handler(AppletEvent::FocusChanged);
            break;
        case AppletHookType_OnOperationMode:
            impl->handler(AppletEvent::OperationModeChanged);
            break;
        case AppletHookType_OnPerformanceMode:
            impl->handler(AppletEvent::PerformanceModeChanged);
            break;
        case AppletHookType_OnExitRequest:
            impl->handler(AppletEvent::ExitRequested);
            break;
        case AppletHookType_OnResume:
            impl->handler(AppletEvent::Resumed);
            break;
        default:
            break;
    }
}
#endif

EventHookRegistration::~EventHookRegistration() {
    unregister();
}

EventHookRegistration::EventHookRegistration(EventHookRegistration&& other) noexcept
    : impl_(std::move(other.impl_)), active_(other.active_) {
    other.active_ = false;
}

EventHookRegistration& EventHookRegistration::operator=(EventHookRegistration&& other) noexcept {
    if (this != &other) {
        unregister();
        impl_ = std::move(other.impl_);
        active_ = other.active_;
        other.active_ = false;
    }
    return *this;
}

void EventHookRegistration::unregister() noexcept {
    if (active_) {
#ifdef __SWITCH__
        if (impl_) {
            appletUnhook(&impl_->cookie);
        }
#endif
        active_ = false;
    }
}

EventHookRegistration register_event_handler(std::function<void(AppletEvent)> handler) {
    EventHookRegistration reg;
    reg.impl_ = std::make_unique<EventHookRegistration::Impl>();
    reg.impl_->handler = std::move(handler);
    reg.active_ = true;

#ifdef __SWITCH__
    appletHook(&reg.impl_->cookie, applet_hook_trampoline, reg.impl_.get());
#endif

    return reg;
}

} // namespace nxdev::power
