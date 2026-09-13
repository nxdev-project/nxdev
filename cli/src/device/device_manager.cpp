#include <nxdev/device/device_manager.hpp>
#include <nxdev/config/host_config.hpp>
#include <nxdev/manifest/yaml.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

#if !defined(_WIN32)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace nxdev::device {

namespace fs = std::filesystem;

DeviceManager::DeviceManager(
    const std::string& custom_config_path,
    const std::string& project_root)
    : project_root_(project_root) {
    if (!custom_config_path.empty()) {
        config_path_ = custom_config_path;
    } else {
        fs::path gdir = config::HostConfig::default_global_config_dir();
        config_path_ = (gdir / "devices.yaml").string();
    }

    reload();
}

void DeviceManager::reload() {
    devices_.clear();

    // 1. Load global devices file
    load_from_file(config_path_);

    // 2. Load project-local devices if available
    if (!project_root_.empty()) {
        fs::path local_path = fs::path(project_root_) / ".nxdev" / "devices.yaml";
        if (fs::exists(local_path)) {
            load_from_file(local_path.string());
        }
    }
}

void DeviceManager::load_from_file(const std::string& path) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::stringstream buffer;
    buffer << file.rdbuf();

    manifest::YamlParseError err;
    manifest::YamlParser parser;
    auto root = parser.parse(buffer.str(), err);
    if (!root || !root->is_mapping()) return;

    const auto* dev_node = root->get("devices");
    if (!dev_node && root->is_mapping()) {
        dev_node = &(*root);
    }

    if (dev_node && dev_node->is_mapping()) {
        for (const auto& [dev_id, node] : dev_node->as_mapping()) {
            if (!node.is_mapping()) continue;
            Device d;
            d.id = dev_id;

            if (const auto* host_n = node.get("host")) {
                d.host = host_n->as_string();
            } else if (const auto* ip_n = node.get("ip")) {
                d.host = ip_n->as_string();
            }

            if (const auto* name_n = node.get("name")) {
                d.name = name_n->as_string();
            } else {
                d.name = d.id;
            }

            if (const auto* tr_n = node.get("transport")) {
                d.transport = tr_n->as_string();
            } else {
                d.transport = "nxlink";
            }

            if (const auto* p_n = node.get("port")) {
                if (auto p = p_n->as_uint()) {
                    d.port = static_cast<uint16_t>(*p);
                }
            }

            if (const auto* def_n = node.get("default")) {
                if (auto b = def_n->as_bool()) {
                    d.is_default = *b;
                }
            }

            if (const auto* c_n = node.get("created_at")) {
                d.created_at = c_n->as_string();
            }

            if (!d.id.empty() && !d.host.empty()) {
                // If device already exists, update it
                auto it = std::find_if(devices_.begin(), devices_.end(),
                    [&](const Device& existing) { return existing.id == d.id; });
                if (it != devices_.end()) {
                    *it = d;
                } else {
                    devices_.push_back(d);
                }
            }
        }
    }
}

bool DeviceManager::save() {
    try {
        fs::path p(config_path_);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        // Write to temporary file first for atomic safety
        fs::path tmp_path = p;
        tmp_path += ".tmp";

        std::ofstream out(tmp_path);
        if (!out.is_open()) return false;

        out << "# NXDev Device Configuration\n";
        out << "devices:\n";
        for (const auto& dev : devices_) {
            out << "  " << dev.id << ":\n";
            out << "    host: \"" << dev.host << "\"\n";
            if (!dev.name.empty() && dev.name != dev.id) {
                out << "    name: \"" << dev.name << "\"\n";
            }
            out << "    transport: \"" << dev.transport << "\"\n";
            if (dev.port > 0) {
                out << "    port: " << dev.port << "\n";
            }
            if (dev.is_default) {
                out << "    default: true\n";
            }
            if (!dev.created_at.empty()) {
                out << "    created_at: \"" << dev.created_at << "\"\n";
            }
        }

        out.close();
        fs::rename(tmp_path, p);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<Device> DeviceManager::list_devices() const {
    return devices_;
}

std::optional<Device> DeviceManager::get_device(const std::string& id) const {
    for (const auto& d : devices_) {
        if (d.id == id) return d;
    }
    return std::nullopt;
}

std::optional<Device> DeviceManager::find_device(const std::string& query) const {
    if (query.empty()) return std::nullopt;
    for (const auto& d : devices_) {
        if (d.id == query || d.name == query || d.host == query) {
            return d;
        }
    }
    return std::nullopt;
}

std::optional<Device> DeviceManager::get_default_device() const {
    for (const auto& d : devices_) {
        if (d.is_default) return d;
    }
    return std::nullopt;
}

bool DeviceManager::add_device(const Device& device, bool set_as_default) {
    if (!Device::is_valid_id(device.id) || !Device::is_valid_host(device.host)) {
        return false;
    }

    Device updated = device;
    if (updated.name.empty()) {
        updated.name = updated.id;
    }
    if (updated.created_at.empty()) {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &tt);
#else
        gmtime_r(&tt, &tm_buf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
        updated.created_at = buf;
    }

    if (set_as_default || devices_.empty()) {
        for (auto& d : devices_) {
            d.is_default = false;
        }
        updated.is_default = true;
    } else if (updated.is_default) {
        for (auto& d : devices_) {
            d.is_default = false;
        }
    }

    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&](const Device& d) { return d.id == updated.id; });
    if (it != devices_.end()) {
        *it = updated;
    } else {
        devices_.push_back(updated);
    }

    return save();
}

bool DeviceManager::remove_device(const std::string& id) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&](const Device& d) { return d.id == id; });
    if (it == devices_.end()) {
        return false;
    }

    bool was_default = it->is_default;
    devices_.erase(it);

    // If default was removed and devices remain, set the first as default
    if (was_default && !devices_.empty()) {
        devices_.front().is_default = true;
    }

    return save();
}

bool DeviceManager::set_default_device(const std::string& id) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&](const Device& d) { return d.id == id; });
    if (it == devices_.end()) {
        return false;
    }

    for (auto& d : devices_) {
        d.is_default = (d.id == id);
    }

    return save();
}

bool DeviceManager::resolve_target(
    const std::string& explicit_host,
    const std::string& explicit_device,
    Device& out_device,
    std::string& out_error_message) const {
    
    // 1. Explicit CLI --host <ip/hostname>
    if (!explicit_host.empty()) {
        if (!Device::is_valid_host(explicit_host)) {
            out_error_message = "Invalid host address syntax: '" + explicit_host + "'";
            return false;
        }
        out_device.id = "cli-target";
        out_device.name = "Explicit Host";
        out_device.host = explicit_host;
        out_device.transport = "nxlink";
        out_device.port = 0;
        out_device.is_default = false;
        return true;
    }

    // 2. Explicit CLI --device <id>
    if (!explicit_device.empty()) {
        auto dev = find_device(explicit_device);
        if (dev) {
            out_device = *dev;
            return true;
        }
        out_error_message = "Specified device '" + explicit_device + "' is not configured. Use 'nxdev devices add' to register it.";
        return false;
    }

    // 3. Global or Project Default device
    auto def = get_default_device();
    if (def) {
        out_device = *def;
        return true;
    }

    // 4. Single configured device fallback
    if (devices_.size() == 1) {
        out_device = devices_.front();
        return true;
    }

    // 5. If multiple devices exist and none is default
    if (devices_.size() > 1) {
        std::ostringstream oss;
        oss << "Multiple devices configured but no default is set. Specify --device <name> or set a default with 'nxdev devices set-default <name>':\n";
        for (const auto& d : devices_) {
            oss << "  - " << d.id << " (" << d.host << ")\n";
        }
        out_error_message = oss.str();
        return false;
    }

    // 6. No devices configured
    out_error_message = "No target Switch device configured. Provide --host <ip>, or register a device with 'nxdev devices add <name> --host <ip>'.";
    return false;
}

bool DeviceManager::test_connectivity(const Device& device, int timeout_ms) {
    if (device.host.empty()) return false;

#if !defined(_WIN32)
    uint16_t target_port = device.port > 0 ? device.port : 28280; // nxlink default port
    
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(target_port);
    if (getaddrinfo(device.host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        return false;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return false;
    }

    // Set non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int conn_res = connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (conn_res == 0) {
        close(sock);
        return true;
    }

    struct pollfd pfd{};
    pfd.fd = sock;
    pfd.events = POLLOUT;

    int poll_res = poll(&pfd, 1, timeout_ms);
    if (poll_res > 0) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        close(sock);
        return (so_error == 0);
    }

    close(sock);
    return false;
#else
    return true; // Graceful fallback on Windows without raw socket overhead
#endif
}

} // namespace nxdev::device
