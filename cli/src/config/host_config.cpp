#include <nxdev/config/host_config.hpp>
#include <nxdev/manifest/yaml.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nxdev::config {

namespace fs = std::filesystem;

std::string HostConfig::default_global_config_dir() {
#if defined(_WIN32)
    if (const char* appdata = std::getenv("APPDATA")) {
        return (fs::path(appdata) / "nxdev").string();
    }
    return (fs::path("C:/Users") / "Default" / "AppData" / "Roaming" / "nxdev").string();
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        return (fs::path(home) / "Library" / "Application Support" / "nxdev").string();
    }
    return "/tmp/nxdev";
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        return (fs::path(xdg) / "nxdev").string();
    }
    if (const char* home = std::getenv("HOME")) {
        return (fs::path(home) / ".config" / "nxdev").string();
    }
    return "/tmp/nxdev";
#endif
}

std::optional<std::string> HostConfig::get_value(const std::string& key) const {
    auto it = raw_entries_.find(key);
    if (it != raw_entries_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::map<std::string, std::string> HostConfig::list_entries() const {
    return raw_entries_;
}

void HostConfig::load_file(const std::string& filepath, bool is_local) {
    if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
        return;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) return;

    std::stringstream buf;
    buf << file.rdbuf();

    manifest::YamlParseError err;
    manifest::YamlParser parser;
    auto root = parser.parse(buf.str(), err);
    if (!root || !root->is_mapping()) return;

    if (is_local) has_local_config_ = true;
    else has_global_config_ = true;

    // toolchain
    const auto* tc = root->get("toolchain");
    if (!tc) tc = root->get("Toolchain");
    if (tc && tc->is_mapping()) {
        const auto* dkp = tc->get("devkitPro");
        if (!dkp) dkp = tc->get("devkitpro");
        if (dkp) {
            toolchain_.devkitpro = dkp->as_string();
            raw_entries_["toolchain.devkitpro"] = dkp->as_string();
        }

        const auto* dka = tc->get("devkitA64");
        if (!dka) dka = tc->get("devkita64");
        if (dka) {
            toolchain_.devkita64 = dka->as_string();
            raw_entries_["toolchain.devkita64"] = dka->as_string();
        }
    }

    // development / deployment
    const auto* dev = root->get("development");
    if (!dev) dev = root->get("deployment");
    if (dev && dev->is_mapping()) {
        const auto* ip = dev->get("switchIp");
        if (!ip) ip = dev->get("switch_ip");
        if (ip) {
            development_.switch_ip = ip->as_string();
            raw_entries_["development.switch_ip"] = ip->as_string();
        }

        const auto* port = dev->get("nxlinkPort");
        if (!port) port = dev->get("nxlink_port");
        if (port) {
            if (auto p = port->as_uint()) {
                development_.nxlink_port = static_cast<uint16_t>(*p);
                raw_entries_["development.nxlink_port"] = std::to_string(*p);
            }
        }
    }
}

void HostConfig::apply_env_overrides() {
    if (const char* dkp = std::getenv("DEVKITPRO")) {
        toolchain_.devkitpro = dkp;
        raw_entries_["toolchain.devkitpro"] = dkp;
    }
    if (const char* dka = std::getenv("DEVKITA64")) {
        toolchain_.devkita64 = dka;
        raw_entries_["toolchain.devkita64"] = dka;
    }
}

HostConfig HostConfig::load(const std::string& project_root) {
    HostConfig config;

    // 1. Global config (~/.config/nxdev/config.yaml)
    fs::path gdir = default_global_config_dir();
    fs::path gfile = gdir / "config.yaml";
    config.global_config_path_ = gfile.string();
    config.load_file(gfile.string(), false);

    // 2. Project-local config (<root>/.nxdev/local.yaml)
    if (!project_root.empty()) {
        fs::path lfile = fs::path(project_root) / ".nxdev" / "local.yaml";
        config.local_config_path_ = lfile.string();
        config.load_file(lfile.string(), true);
    }

    return config;
}

} // namespace nxdev::config
