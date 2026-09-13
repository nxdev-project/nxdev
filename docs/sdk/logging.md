# Logging Subsystem (`nxdev::log`)

The `nxdev::log` namespace provides a thread-safe, structured logging facility for NXDev applications.

## Log Levels

```cpp
namespace nxdev::log {
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Off
    };
}
```

The default log level is `Level::Info`.

---

## Basic Logging

```cpp
#include <nxdev/log.hpp>

nxdev::log::trace("Detailed execution trace");
nxdev::log::debug("Variable value: " + std::to_string(x));
nxdev::log::info("Subsystem initialized");
nxdev::log::warn("Low memory warning");
nxdev::log::error("Failed to connect to network socket");
```

---

## Log Filtering

Set the minimum log level:

```cpp
nxdev::log::set_level(nxdev::log::Level::Debug);
```

To silence all logging completely:

```cpp
nxdev::log::set_level(nxdev::log::Level::Off);
```

---

## Custom Log Sinks

By default, logs are written to `stdout`/`stderr` using `ConsoleLogSink` with formatted prefixes:
`[NXDev][INFO] Subsystem initialized`

You can attach a custom sink (such as for file logging, in-game console overlays, or network logging via `nxlink`):

```cpp
// Using a lambda or function callback:
nxdev::log::set_custom_sink([](nxdev::log::Level level, std::string_view message) {
    // Send to in-game console HUD or UDP socket
    my_hud_console.append_line(message);
});

// Or using an ILogSink implementation:
class NetworkLogSink : public nxdev::log::ILogSink {
public:
    void write(nxdev::log::Level level, std::string_view formatted_message) override {
        send_udp(formatted_message);
    }
};

nxdev::log::set_sink(std::make_shared<NetworkLogSink>());

// Reset back to default console sink:
nxdev::log::reset_sink();
```
