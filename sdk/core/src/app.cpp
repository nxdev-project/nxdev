#include <nxdev/app.hpp>
#include <atomic>

namespace nxdev {

struct App::Impl {
    std::atomic<bool> initialized{false};
    std::atomic<bool> running{false};
};

App::App() : impl_(std::make_unique<Impl>()) {}

App::~App() {
    finalize();
}

App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

Result App::initialize() {
    if (impl_->initialized.load()) {
        return results::AlreadyInitialized;
    }
    // Note: libnx service initialization will be invoked conditionally here in later stages
    impl_->initialized.store(true);
    impl_->running.store(true);
    return results::Success;
}

bool App::is_running() const noexcept {
    return impl_->initialized.load() && impl_->running.load();
}

void App::request_exit() noexcept {
    impl_->running.store(false);
}

void App::finalize() {
    if (impl_ && impl_->initialized.load()) {
        impl_->running.store(false);
        impl_->initialized.store(false);
    }
}

} // namespace nxdev
