#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <functional>

namespace nxdev::log {

enum class Level {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Off
};

[[nodiscard]] std::string_view level_to_string(Level level) noexcept;

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(Level level, std::string_view formatted_message) = 0;
};

class ConsoleLogSink : public ILogSink {
public:
    ConsoleLogSink() = default;
    ~ConsoleLogSink() override = default;
    void write(Level level, std::string_view formatted_message) override;
};

void set_level(Level level) noexcept;
[[nodiscard]] Level get_level() noexcept;

void set_sink(std::shared_ptr<ILogSink> sink) noexcept;
void set_custom_sink(std::function<void(Level, std::string_view)> callback) noexcept;
void reset_sink() noexcept;

void write(Level level, std::string_view message);

inline void trace(std::string_view message) {
    write(Level::Trace, message);
}

inline void debug(std::string_view message) {
    write(Level::Debug, message);
}

inline void info(std::string_view message) {
    write(Level::Info, message);
}

inline void warn(std::string_view message) {
    write(Level::Warn, message);
}

inline void error(std::string_view message) {
    write(Level::Error, message);
}

} // namespace nxdev::log
