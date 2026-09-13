#include <nxdev/log.hpp>
#include <iostream>
#include <mutex>
#include <atomic>

namespace nxdev::log {

std::string_view level_to_string(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        case Level::Off:   return "OFF";
    }
    return "UNKNOWN";
}

void ConsoleLogSink::write(Level level, std::string_view formatted_message) {
    if (level == Level::Error || level == Level::Warn) {
        std::cerr << formatted_message << "\n";
    } else {
        std::cout << formatted_message << "\n";
    }
}

namespace {
    class CustomCallbackSink : public ILogSink {
    public:
        explicit CustomCallbackSink(std::function<void(Level, std::string_view)> cb)
            : cb_(std::move(cb)) {}
        void write(Level level, std::string_view formatted_message) override {
            if (cb_) {
                cb_(level, formatted_message);
            }
        }
    private:
        std::function<void(Level, std::string_view)> cb_;
    };

    struct LoggerState {
        std::atomic<Level> level{Level::Info};
        std::mutex mutex;
        std::shared_ptr<ILogSink> sink = std::make_shared<ConsoleLogSink>();
    };

    LoggerState& get_state() {
        static LoggerState state;
        return state;
    }
}

void set_level(Level level) noexcept {
    get_state().level.store(level);
}

Level get_level() noexcept {
    return get_state().level.load();
}

void set_sink(std::shared_ptr<ILogSink> sink) noexcept {
    auto& state = get_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (sink) {
        state.sink = std::move(sink);
    } else {
        state.sink = std::make_shared<ConsoleLogSink>();
    }
}

void set_custom_sink(std::function<void(Level, std::string_view)> callback) noexcept {
    if (callback) {
        set_sink(std::make_shared<CustomCallbackSink>(std::move(callback)));
    } else {
        reset_sink();
    }
}

void reset_sink() noexcept {
    set_sink(std::make_shared<ConsoleLogSink>());
}

void write(Level level, std::string_view message) {
    auto& state = get_state();
    Level cur = state.level.load();
    if (level < cur || cur == Level::Off) {
        return;
    }

    std::string formatted = "[NXDev][" + std::string(level_to_string(level)) + "] " + std::string(message);

    std::shared_ptr<ILogSink> sink_copy;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        sink_copy = state.sink;
    }

    if (sink_copy) {
        sink_copy->write(level, formatted);
    }
}

} // namespace nxdev::log
