#pragma once

#include <utility>
#include <type_traits>

namespace nxdev {

/**
 * @brief RAII Scope Guard that executes a callable when exiting scope unless cancelled.
 */
template <typename F>
class ScopeExit {
public:
    explicit ScopeExit(F&& func) : func_(std::forward<F>(func)), active_(true) {}
    explicit ScopeExit(const F& func) : func_(func), active_(true) {}

    ~ScopeExit() {
        if (active_) {
            func_();
        }
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    ScopeExit(ScopeExit&& other) noexcept : func_(std::move(other.func_)), active_(other.active_) {
        other.active_ = false;
    }

    ScopeExit& operator=(ScopeExit&& other) noexcept {
        if (this != &other) {
            if (active_) {
                func_();
            }
            func_ = std::move(other.func_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    /**
     * @brief Cancel the scope exit action so it will not execute upon destruction.
     */
    void cancel() noexcept {
        active_ = false;
    }

    /**
     * @brief Release the guard without executing the action.
     */
    void release() noexcept {
        active_ = false;
    }

    [[nodiscard]] bool is_active() const noexcept {
        return active_;
    }

private:
    F func_;
    bool active_{false};
};

/**
 * @brief Factory function for creating a ScopeExit guard.
 */
template <typename F>
[[nodiscard]] auto scope_exit(F&& func) {
    return ScopeExit<std::decay_t<F>>(std::forward<F>(func));
}

/**
 * @brief Alias helper for make_scope_exit.
 */
template <typename F>
[[nodiscard]] auto make_scope_exit(F&& func) {
    return ScopeExit<std::decay_t<F>>(std::forward<F>(func));
}

} // namespace nxdev
