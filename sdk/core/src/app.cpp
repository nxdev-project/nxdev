#include <nxdev/app.hpp>
#include <atomic>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev {

std::string_view applet_type_to_string(AppletType type) noexcept {
    switch (type) {
        case AppletType::None: return "None";
        case AppletType::Default: return "Default";
        case AppletType::Application: return "Application";
        case AppletType::SystemApplet: return "SystemApplet";
        case AppletType::LibraryApplet: return "LibraryApplet";
        case AppletType::OverlayApplet: return "OverlayApplet";
        case AppletType::Unknown: return "Unknown";
    }
    return "Unknown";
}

struct App::Impl {
    std::atomic<bool> initialized{false};
    std::atomic<bool> exit_requested{false};
};

App::App() : impl_(std::make_unique<Impl>()) {}

App::~App() {
    finalize();
}

App::App(App&& other) noexcept = default;
App& App::operator=(App&& other) noexcept = default;

Result<App> App::create() {
    App app;
    auto res = app.initialize();
    if (res.failed()) {
        return Result<App>(res.code());
    }
    return Result<App>(std::move(app));
}

Result<void> App::initialize() {
    if (impl_->initialized.load()) {
        return Result<void>(results::AlreadyInitialized);
    }
    impl_->initialized.store(true);
    impl_->exit_requested.store(false);
    return Result<void>::Success();
}

bool App::running() const noexcept {
    if (!impl_ || !impl_->initialized.load() || impl_->exit_requested.load()) {
        return false;
    }

#ifdef __SWITCH__
    return appletMainLoop();
#else
    return true;
#endif
}

void App::request_exit() noexcept {
    if (impl_) {
        impl_->exit_requested.store(true);
    }
}

bool App::exit_requested() const noexcept {
    return impl_ ? impl_->exit_requested.load() : true;
}

AppletType App::applet_type() const noexcept {
#ifdef __SWITCH__
    ::AppletType t = appletGetAppletType();
    switch (t) {
        case AppletType_None: return AppletType::None;
        case AppletType_Default: return AppletType::Default;
        case AppletType_Application: return AppletType::Application;
        case AppletType_SystemApplet: return AppletType::SystemApplet;
        case AppletType_LibraryApplet: return AppletType::LibraryApplet;
        case AppletType_OverlayApplet: return AppletType::OverlayApplet;
        default: return AppletType::Unknown;
    }
#else
    return AppletType::Application;
#endif
}

void App::finalize() noexcept {
    if (impl_ && impl_->initialized.load()) {
        impl_->exit_requested.store(true);
        impl_->initialized.store(false);
    }
}

} // namespace nxdev
