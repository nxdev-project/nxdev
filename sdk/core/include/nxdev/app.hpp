#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <string_view>
#include <memory>

namespace nxdev {

/**
 * @brief High-level application lifecycle manager for Switch homebrew apps.
 * Wraps applet/mainLoop concepts cleanly without obfuscating raw libnx when needed.
 */
class App {
public:
    App();
    virtual ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) noexcept;
    App& operator=(App&&) noexcept;

    /**
     * @brief Initialize SDK core subsystems and runtime.
     */
    Result initialize();

    /**
     * @brief Check whether the application should continue running (e.g. appletMainLoop).
     */
    [[nodiscard]] bool is_running() const noexcept;

    /**
     * @brief Signal the application loop to terminate cleanly.
     */
    void request_exit() noexcept;

    /**
     * @brief Cleanup SDK subsystems before process termination.
     */
    void finalize();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace nxdev
