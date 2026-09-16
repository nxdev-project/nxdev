#include <nxdev/ui/application.hpp>
#include <nxdev/log.hpp>
#include "internal.hpp"
#include <deque>
#include <mutex>
#include <exception>

namespace nxdev::ui {

struct Application::Impl {
    std::shared_ptr<View> root_view;
    std::vector<std::shared_ptr<View>> screen_stack;
    std::deque<std::function<void()>> task_queue;
    std::mutex task_mutex;
    bool exit_requested = false;

    void process_dispatched_tasks() {
        std::vector<std::function<void()>> tasks;
        {
            std::lock_guard<std::mutex> lock(task_mutex);
            while (!task_queue.empty()) {
                tasks.push_back(std::move(task_queue.front()));
                task_queue.pop_front();
            }
        }
        for (auto& task : tasks) {
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    nxdev::log::error("Exception in dispatched UI task: " + std::string(e.what()));
                } catch (...) {
                    nxdev::log::error("Unknown exception in dispatched UI task");
                }
            }
        }
    }
};

static Application::Impl* s_active_app_impl = nullptr;

Application::Application(Config config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {
    s_active_app_impl = impl_.get();
}

Application::~Application() {
    if (s_active_app_impl == impl_.get()) {
        s_active_app_impl = nullptr;
    }
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    // Clean Borealis teardown
#endif
}

Application::Application(Application&& other) noexcept = default;
Application& Application::operator=(Application&& other) noexcept = default;

nxdev::Result<Application> Application::create() {
    return create(Config{});
}

nxdev::Result<Application> Application::create(const Config& config) {
    try {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
#ifdef BRLS_RESOURCES
        nxdev::log::debug("[NXDev::Borealis] BRLS_RESOURCES=" + std::string(BRLS_RESOURCES));
#endif
        // Initialize Borealis backend
        if (!brls::Application::init()) {
            nxdev::log::error("Failed to initialize Borealis UI runtime");
            return Result<Application>(results::NotInitialized);
        }

        brls::Application::createWindow(config.name);
#endif
        return Result<Application>(Application(config));
    } catch (const std::exception& e) {
        nxdev::log::error("Borealis backend initialization threw exception: " + std::string(e.what()));
        return Result<Application>(results::NotInitialized);
    } catch (...) {
        nxdev::log::error("Borealis backend initialization threw unknown exception");
        return Result<Application>(results::NotInitialized);
    }
}

nxdev::Result<void> Application::set_root(std::shared_ptr<View> view) {
    if (!view) {
        return Result<void>(results::InvalidArgument);
    }
    impl_->root_view = view;
    impl_->screen_stack.clear();
    impl_->screen_stack.push_back(view);

#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(view->native_handle())) {
        auto* act = new brls::Activity(v);
        brls::Application::pushActivity(act);
    }
#endif
    return Result<void>::Success();
}

nxdev::Result<void> Application::push_screen(std::shared_ptr<View> screen) {
    if (!screen) {
        return Result<void>(results::InvalidArgument);
    }
    impl_->screen_stack.push_back(screen);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(screen->native_handle())) {
        auto* act = new brls::Activity(v);
        brls::Application::pushActivity(act);
    }
#endif
    return Result<void>::Success();
}

nxdev::Result<void> Application::pop_screen() {
    if (impl_->screen_stack.size() <= 1) {
        return Result<void>(results::NotFound);
    }
    impl_->screen_stack.pop_back();
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    brls::Application::popActivity();
#endif
    return Result<void>::Success();
}

nxdev::Result<void> Application::run() {
    running_ = true;
    try {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
        while (running_ && !impl_->exit_requested && brls::Application::mainLoop()) {
            impl_->process_dispatched_tasks();
        }
#else
        // Mock host loop
        while (running_ && !impl_->exit_requested) {
            impl_->process_dispatched_tasks();
            break; // Terminate cleanly after one step in test/mock mode
        }
#endif
        running_ = false;
        return Result<void>::Success();
    } catch (const std::exception& e) {
        running_ = false;
        nxdev::log::error("Unhandled exception in UI main loop: " + std::string(e.what()));
        return Result<void>(results::NotInitialized);
    } catch (...) {
        running_ = false;
        nxdev::log::error("Unknown unhandled exception in UI main loop");
        return Result<void>(results::NotInitialized);
    }
}

void Application::request_exit() {
    if (impl_) {
        impl_->exit_requested = true;
    }
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    brls::Application::quit();
#endif
    running_ = false;
}

bool Application::is_running() const {
    return running_;
}

void Application::dispatch(std::function<void()> task) {
    if (s_active_app_impl) {
        std::lock_guard<std::mutex> lock(s_active_app_impl->task_mutex);
        s_active_app_impl->task_queue.push_back(std::move(task));
    }
}

BackendInfo Application::backend_info() {
    return BackendInfo{
        .backend_name = "borealis",
        .repository = "https://github.com/jvrcruzGAMES/borealis",
        .revision = "5f08b286f3df737f3321d2247a6fe633fcead03c",
        .api_version = 1
    };
}

void* Application::native_handle() const {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    return brls::Application::getPlatform();
#else
    return nullptr;
#endif
}

} // namespace nxdev::ui
