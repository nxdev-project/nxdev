#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <string>
#include <string_view>
#include <memory>

namespace nxdev::network {

namespace results {
    inline constexpr ResultCode ServiceInitFailed{0x00040001};
    inline constexpr ResultCode NotConnected{0x00040002};
    inline constexpr ResultCode InvalidOptions{0x00040003};
}

enum class BsdServiceType : u8 {
    User   = 1,
    System = 2,
    Auto   = 3
};

struct SocketOptions {
    u32 tcp_tx_buf_size{0x8000};
    u32 tcp_rx_buf_size{0x10000};
    u32 tcp_tx_buf_max_size{0x40000};
    u32 tcp_rx_buf_max_size{0x40000};
    u32 udp_tx_buf_size{0x2400};
    u32 udp_rx_buf_size{0xA500};
    u32 sb_efficiency{4};
    u32 num_bsd_sessions{3};
    BsdServiceType bsd_service_type{BsdServiceType::User};

    [[nodiscard]] bool is_valid() const noexcept {
        return tcp_tx_buf_size > 0 && tcp_rx_buf_size > 0 &&
               udp_tx_buf_size > 0 && udp_rx_buf_size > 0 &&
               sb_efficiency > 0 && num_bsd_sessions > 0;
    }
};

/**
 * @brief RAII Reference-counted manager for Nintendo Switch BSD socket subsystem.
 */
class SocketService {
public:
    SocketService() noexcept : active_(false) {}
    ~SocketService();

    SocketService(const SocketService&) = delete;
    SocketService& operator=(const SocketService&) = delete;

    SocketService(SocketService&& other) noexcept;
    SocketService& operator=(SocketService&& other) noexcept;

    /**
     * @brief Initializes the socket driver with custom buffer options.
     */
    [[nodiscard]] static Result<SocketService> create(const SocketOptions& options);

    /**
     * @brief Initializes the socket driver with standard homebrew default settings.
     */
    [[nodiscard]] static Result<SocketService> create_default();

    [[nodiscard]] bool is_active() const noexcept {
        return active_;
    }

    explicit operator bool() const noexcept {
        return active_;
    }

    void close() noexcept;

private:
    explicit SocketService(bool active) noexcept : active_(active) {}
    bool active_{false};
};

/**
 * @brief Alias Context for SocketService.
 */
using Context = SocketService;

/**
 * @brief Queries whether the console currently has active network connectivity.
 */
[[nodiscard]] bool is_connected() noexcept;

/**
 * @brief Queries the assigned local IPv4 address (e.g. "192.168.1.50").
 */
[[nodiscard]] Result<std::string> get_local_ip() noexcept;

} // namespace nxdev::network
