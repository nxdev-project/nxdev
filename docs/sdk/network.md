# Network & Socket Lifecycle (`NXDev::Network`)

The `NXDev::Network` module provides RAII lifecycle management for the Nintendo Switch BSD socket driver (`bsd:*`) and network status queries.

> [!NOTE]
> `NXDev::Network` manages the initialization and teardown of the native socket subsystem. It is not an HTTP client library. Standard POSIX/BSD socket functions (`socket()`, `bind()`, `connect()`, `send()`, `recv()`) are used directly once the subsystem is initialized.

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.network
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Network)
```

## Header

```cpp
#include <nxdev/network.hpp>
```

---

## RAII Socket Service

```cpp
#include <nxdev/network.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void network_task() {
    // Initializes the socket driver with standard homebrew buffer sizes:
    auto net = nxdev::network::SocketService::create_default();
    if (net.failed()) {
        return;
    }

    // Check connectivity:
    if (nxdev::network::is_connected()) {
        auto ip_res = nxdev::network::get_local_ip();
        if (ip_res.is_success()) {
            std::cout << "Assigned IP: " << ip_res.value() << "\n";
        }
    }

    // Use standard BSD socket APIs:
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        // ... perform socket operations ...
        close(sock);
    }

    // Socket driver is cleanly deinitialized when `net` leaves scope.
}
```

---

## Configurable Buffer Options

If your application requires custom buffer sizes:

```cpp
nxdev::network::SocketOptions options;
options.tcp_tx_buf_size = 0x10000; // 64 KB
options.tcp_rx_buf_size = 0x20000; // 128 KB
options.sb_efficiency = 4;

auto net = nxdev::network::SocketService::create(options);
```
