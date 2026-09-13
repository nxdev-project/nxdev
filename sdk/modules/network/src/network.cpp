#include <nxdev/network.hpp>
#include <mutex>
#include <sstream>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::network {

struct SocketRefCounter {
    std::mutex mutex;
    int ref_count{0};

    bool retain(const SocketOptions* custom_options = nullptr) {
        std::lock_guard<std::mutex> lock(mutex);
#ifdef __SWITCH__
        if (ref_count == 0) {
            ::Result rc = 0;
            if (custom_options) {
                SocketInitConfig cfg{};
                cfg.tcp_tx_buf_size = custom_options->tcp_tx_buf_size;
                cfg.tcp_rx_buf_size = custom_options->tcp_rx_buf_size;
                cfg.tcp_tx_buf_max_size = custom_options->tcp_tx_buf_max_size;
                cfg.tcp_rx_buf_max_size = custom_options->tcp_rx_buf_max_size;
                cfg.udp_tx_buf_size = custom_options->udp_tx_buf_size;
                cfg.udp_rx_buf_size = custom_options->udp_rx_buf_size;
                cfg.sb_efficiency = custom_options->sb_efficiency;
                cfg.num_bsd_sessions = custom_options->num_bsd_sessions;
                cfg.bsd_service_type = static_cast<::BsdServiceType>(custom_options->bsd_service_type);
                rc = socketInitialize(&cfg);
            } else {
                rc = socketInitializeDefault();
            }

            if (R_FAILED(rc)) {
                return false;
            }
        }
#else
        (void)custom_options;
#endif
        ref_count++;
        return true;
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex);
        if (ref_count > 0) {
            ref_count--;
#ifdef __SWITCH__
            if (ref_count == 0) {
                socketExit();
            }
#endif
        }
    }
};

static SocketRefCounter g_sock_ref_counter;

SocketService::~SocketService() {
    close();
}

SocketService::SocketService(SocketService&& other) noexcept
    : active_(other.active_) {
    other.active_ = false;
}

SocketService& SocketService::operator=(SocketService&& other) noexcept {
    if (this != &other) {
        close();
        active_ = other.active_;
        other.active_ = false;
    }
    return *this;
}

Result<SocketService> SocketService::create(const SocketOptions& options) {
    if (!options.is_valid()) {
        return Result<SocketService>(results::InvalidOptions);
    }
    if (!g_sock_ref_counter.retain(&options)) {
        return Result<SocketService>(results::ServiceInitFailed);
    }
    return Result<SocketService>(SocketService(true));
}

Result<SocketService> SocketService::create_default() {
    if (!g_sock_ref_counter.retain(nullptr)) {
        return Result<SocketService>(results::ServiceInitFailed);
    }
    return Result<SocketService>(SocketService(true));
}

void SocketService::close() noexcept {
    if (active_) {
        g_sock_ref_counter.release();
        active_ = false;
    }
}

bool is_connected() noexcept {
#ifdef __SWITCH__
    ::Result rc = nifmInitialize(NifmServiceType_User);
    if (R_FAILED(rc)) {
        return false;
    }
    u32 ip = 0;
    rc = nifmGetCurrentIpAddress(&ip);
    nifmExit();
    return R_SUCCEEDED(rc) && ip != 0;
#else
    return true;
#endif
}

Result<std::string> get_local_ip() noexcept {
#ifdef __SWITCH__
    ::Result rc = nifmInitialize(NifmServiceType_User);
    if (R_FAILED(rc)) {
        return Result<std::string>(ResultCode(static_cast<u32>(rc)));
    }

    u32 ip = 0;
    rc = nifmGetCurrentIpAddress(&ip);
    nifmExit();

    if (R_FAILED(rc) || ip == 0) {
        return Result<std::string>(results::NotConnected);
    }

    u8 b1 = static_cast<u8>((ip >> 0) & 0xFF);
    u8 b2 = static_cast<u8>((ip >> 8) & 0xFF);
    u8 b3 = static_cast<u8>((ip >> 16) & 0xFF);
    u8 b4 = static_cast<u8>((ip >> 24) & 0xFF);

    std::ostringstream ss;
    ss << static_cast<int>(b1) << "."
       << static_cast<int>(b2) << "."
       << static_cast<int>(b3) << "."
       << static_cast<int>(b4);
    return Result<std::string>(ss.str());
#else
    return Result<std::string>("127.0.0.1");
#endif
}

} // namespace nxdev::network
