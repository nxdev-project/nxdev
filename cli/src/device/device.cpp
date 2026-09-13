#include <nxdev/device/device.hpp>
#include <regex>
#include <sstream>

namespace nxdev::device {

bool Device::is_valid_id(const std::string& id) noexcept {
    if (id.empty() || id.size() > 64) {
        return false;
    }
    // Must start with alphanumeric and contain only [a-zA-Z0-9_-]
    static const std::regex kIdPattern("^[a-zA-Z0-9][a-zA-Z0-9_-]*$");
    return std::regex_match(id, kIdPattern);
}

bool Device::is_valid_host(const std::string& host) noexcept {
    if (host.empty() || host.size() > 253) {
        return false;
    }

    // IPv4 pattern: 4 octets 0-255
    static const std::regex kIpv4Pattern(
        "^([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})$");
    std::smatch match;
    if (std::regex_match(host, match, kIpv4Pattern)) {
        for (size_t i = 1; i <= 4; ++i) {
            int octet = std::stoi(match[i].str());
            if (octet < 0 || octet > 255) return false;
        }
        return true;
    }

    // Bracketed IPv6 e.g. [::1] or bare IPv6 e.g. fe80::1
    if (host.find(':') != std::string::npos) {
        std::string bare = host;
        if (bare.front() == '[' && bare.back() == ']') {
            bare = bare.substr(1, bare.size() - 2);
        }
        static const std::regex kIpv6Pattern("^[0-9a-fA-F:]+$");
        return std::regex_match(bare, kIpv6Pattern);
    }

    // Standard Hostname / FQDN e.g. "switch.local", "my-switch"
    static const std::regex kHostnamePattern(
        "^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])"
        "(\\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*$");
    return std::regex_match(host, kHostnamePattern);
}

std::string Device::display_summary() const {
    std::ostringstream oss;
    oss << id << " (" << host;
    if (port > 0) {
        oss << ":" << port;
    }
    oss << ")";
    if (is_default) {
        oss << " [default]";
    }
    if (!name.empty() && name != id) {
        oss << " - " << name;
    }
    return oss.str();
}

} // namespace nxdev::device
