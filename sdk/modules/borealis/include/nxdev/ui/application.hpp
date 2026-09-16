#pragma once

#include <nxdev/ui/view.hpp>
#include <nxdev/result.hpp>
#include <string>
#include <memory>
#include <functional>

namespace nxdev::ui {

/**
 * @brief Backend revision and diagnostic metadata.
 */
struct BackendInfo {
    std::string_view backend_name;
    std::string_view repository;
    std::string_view revision;
    uint32_t api_version = 1;
};

/**
 * @brief Main UI runtime and application coordinator.
 *
 * Manages the screen stack, event processing loop, and graphics context
 * via RAII lifecycle management.
 */
class Application {
public:
    struct Config {
        std::string name = "NXDev Application";
        std::string resources_dir = "romfs:/";
        bool sound = true;
    };

    ~Application();

    // Non-copyable, movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) noexcept;
    Application& operator=(Application&&) noexcept;

    struct Impl;

    // Factory
    static nxdev::Result<Application> create();
    static nxdev::Result<Application> create(const Config& config);

    // Screen navigation stack
    nxdev::Result<void> set_root(std::shared_ptr<View> view);
    nxdev::Result<void> push_screen(std::shared_ptr<View> screen);
    nxdev::Result<void> pop_screen();

    // Execution lifecycle
    [[nodiscard]] nxdev::Result<void> run();
    void request_exit();
    [[nodiscard]] bool is_running() const;

    // Main thread async dispatching
    static void dispatch(std::function<void()> task);

    // Diagnostics
    [[nodiscard]] static BackendInfo backend_info();

    // Backend Native Handle (unstable escape hatch)
    [[nodiscard]] void* native_handle() const;

private:
    explicit Application(Config config);

    std::unique_ptr<Impl> impl_;
    Config config_;
    bool running_{false};
};

} // namespace nxdev::ui
