#pragma once

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <cstdint>

namespace nxdev::config {

struct ToolchainConfig {
    std::optional<std::string> devkitpro;
    std::optional<std::string> devkita64;
};

struct DevelopmentConfig {
    std::optional<std::string> switch_ip;
    std::optional<uint16_t> nxlink_port;
};

class HostConfig {
public:
    HostConfig() = default;
    ~HostConfig() = default;

    [[nodiscard]] const ToolchainConfig& toolchain() const noexcept { return toolchain_; }
    [[nodiscard]] const DevelopmentConfig& development() const noexcept { return development_; }

    [[nodiscard]] const std::string& global_config_path() const noexcept { return global_config_path_; }
    [[nodiscard]] const std::string& local_config_path() const noexcept { return local_config_path_; }

    [[nodiscard]] bool has_global_config() const noexcept { return has_global_config_; }
    [[nodiscard]] bool has_local_config() const noexcept { return has_local_config_; }

    [[nodiscard]] std::optional<std::string> get_value(const std::string& key) const;
    [[nodiscard]] std::map<std::string, std::string> list_entries() const;

    /**
     * @brief Resolves default global config directory based on OS.
     */
    [[nodiscard]] static std::string default_global_config_dir();

    /**
     * @brief Loads global configuration, optional project-local configuration (.nxdev/local.yaml),
     * and environment overrides.
     */
    static HostConfig load(const std::string& project_root = "");

private:
    std::string global_config_path_;
    std::string local_config_path_;
    bool has_global_config_{false};
    bool has_local_config_{false};

    ToolchainConfig toolchain_;
    DevelopmentConfig development_;
    std::map<std::string, std::string> raw_entries_;

    void load_file(const std::string& filepath, bool is_local);
    void apply_env_overrides();
};

} // namespace nxdev::config
