#pragma once

#include <nxdev/result.hpp>
#include <utility>
#include <functional>

namespace nxdev {

/**
 * @brief Generic RAII wrapper for managing service initialization and cleanup.
 */
template <typename ExitFn>
class ServiceGuard {
public:
    ServiceGuard() noexcept : active_(false) {}

    explicit ServiceGuard(ExitFn exit_fn) noexcept
        : exit_fn_(std::move(exit_fn)), active_(true) {}

    ~ServiceGuard() {
        reset();
    }

    ServiceGuard(const ServiceGuard&) = delete;
    ServiceGuard& operator=(const ServiceGuard&) = delete;

    ServiceGuard(ServiceGuard&& other) noexcept
        : exit_fn_(std::move(other.exit_fn_)), active_(other.active_) {
        other.active_ = false;
    }

    ServiceGuard& operator=(ServiceGuard&& other) noexcept {
        if (this != &other) {
            reset();
            exit_fn_ = std::move(other.exit_fn_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool is_initialized() const noexcept {
        return active_;
    }

    [[nodiscard]] bool is_active() const noexcept {
        return active_;
    }

    explicit operator bool() const noexcept {
        return active_;
    }

    void reset() noexcept {
        if (active_) {
            exit_fn_();
            active_ = false;
        }
    }

    void release() noexcept {
        active_ = false;
    }

private:
    ExitFn exit_fn_{};
    bool active_{false};
};

/**
 * @brief Helper to initialize a service and return a ServiceGuard on success.
 */
template <typename InitFn, typename ExitFn>
[[nodiscard]] Result<ServiceGuard<ExitFn>> make_service_guard(InitFn init_fn, ExitFn exit_fn) {
    auto res = init_fn();
    if (res != 0) {
        return Result<ServiceGuard<ExitFn>>(ResultCode(static_cast<u32>(res)));
    }
    return Result<ServiceGuard<ExitFn>>(ServiceGuard<ExitFn>(std::move(exit_fn)));
}

} // namespace nxdev
