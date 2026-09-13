#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <memory>
#include <string_view>

namespace nxdev {

enum class AppletType {
    None,
    Default,
    Application,
    SystemApplet,
    LibraryApplet,
    OverlayApplet,
    Unknown
};

std::string_view applet_type_to_string(AppletType type) noexcept;

/**
 * @brief High-level application lifecycle manager for Switch homebrew apps.
 * Wraps appletMainLoop() / Horizon OS runtime hooks cleanly without obscuring raw libnx.
 */
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&& other) noexcept;
    App& operator=(App&& other) noexcept;

    /**
     * @brief Factory creating an initialized App instance.
     */
    [[nodiscard]] static Result<App> create();

    /**
     * @brief Initialize SDK core subsystems and runtime.
     */
    Result<void> initialize();

    /**
     * @brief Check whether the application should continue running.
     * On Nintendo Switch, integrates directly with appletMainLoop().
     */
    [[nodiscard]] bool running() const noexcept;

    /**
     * @brief Alias for running().
     */
    [[nodiscard]] bool is_running() const noexcept {
        return running();
    }

    /**
     * @brief Signal the application loop to terminate cleanly.
     */
    void request_exit() noexcept;

    /**
     * @brief Returns whether an exit has been requested.
     */
    [[nodiscard]] bool exit_requested() const noexcept;

    /**
     * @brief Queries the active applet type.
     */
    [[nodiscard]] AppletType applet_type() const noexcept;

    /**
     * @brief Cleanup SDK subsystems before process termination.
     */
    void finalize() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace nxdev
