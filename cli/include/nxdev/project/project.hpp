#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <nxdev/manifest/diagnostics.hpp>
#include <string>
#include <optional>
#include <memory>

namespace nxdev::project {

class NXDevProject {
public:
    NXDevProject() = default;
    ~NXDevProject() = default;

    [[nodiscard]] const std::string& root_path() const noexcept { return root_path_; }
    [[nodiscard]] const std::string& manifest_path() const noexcept { return manifest_path_; }
    [[nodiscard]] const std::string& build_dir() const noexcept { return build_dir_; }
    [[nodiscard]] const std::string& local_config_path() const noexcept { return local_config_path_; }

    [[nodiscard]] bool is_valid() const noexcept { return is_valid_; }
    [[nodiscard]] const manifest::Manifest& manifest() const { return manifest_; }
    [[nodiscard]] const manifest::DiagnosticCollector& diagnostics() const noexcept { return diagnostics_; }

    [[nodiscard]] std::string info_summary() const;
    [[nodiscard]] std::string info_json() const;

    /**
     * @brief Discovers and loads an NXDev project by searching start_dir and walking upward.
     */
    static std::optional<NXDevProject> discover(const std::string& start_dir = "");

    /**
     * @brief Loads an NXDev project from an explicit directory or nxapp.yaml file path.
     */
    static std::optional<NXDevProject> load_explicit(const std::string& path);

private:
    std::string root_path_;
    std::string manifest_path_;
    std::string build_dir_;
    std::string local_config_path_;

    manifest::Manifest manifest_;
    manifest::DiagnosticCollector diagnostics_;
    bool is_valid_{false};
};

} // namespace nxdev::project
