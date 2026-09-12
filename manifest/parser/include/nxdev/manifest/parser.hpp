#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <nxdev/manifest/diagnostics.hpp>
#include <string>
#include <optional>
#include <filesystem>

namespace nxdev::manifest {

struct ParseResult {
    bool success{false};
    std::optional<Manifest> manifest;
    DiagnosticCollector diagnostics;

    [[nodiscard]] bool has_value() const noexcept { return success && manifest.has_value(); }
    [[nodiscard]] const Manifest& value() const { return *manifest; }
    [[nodiscard]] const DiagnosticCollector& diagnostic_collector() const noexcept { return diagnostics; }
};

class Parser {
public:
    static constexpr const char* DEFAULT_MANIFEST_FILENAME = "nxapp.yaml";

    Parser() = default;
    ~Parser() = default;

    /**
     * @brief Parses and validates an nxapp.yaml file from a specific filesystem path.
     */
    [[nodiscard]] ParseResult parse_file(const std::string& filepath);

    /**
     * @brief Parses and validates an nxapp.yaml string with an explicit project directory for asset resolution.
     */
    [[nodiscard]] ParseResult parse_string(const std::string& content, const std::string& project_dir = "");

    /**
     * @brief Discovers nxapp.yaml by searching start_dir and walking upward to parent directories.
     */
    [[nodiscard]] std::optional<std::string> discover_manifest(const std::string& start_dir = "");

    /**
     * @brief Discovers and loads the nearest nxapp.yaml.
     */
    [[nodiscard]] ParseResult load_from_discovery(const std::string& start_dir = "");
};

} // namespace nxdev::manifest
