#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <nxdev/manifest/diagnostics.hpp>
#include <nxdev/manifest/yaml.hpp>

namespace nxdev::manifest {

class ManifestValidator {
public:
    ManifestValidator() = default;
    ~ManifestValidator() = default;

    [[nodiscard]] std::optional<Manifest> validate_and_normalize(
        const YamlNode& root_node,
        const std::string& manifest_filepath,
        const std::string& project_dir,
        DiagnosticCollector& diagnostics
    );

private:
    void validate_schema_version(const YamlNode& root, Manifest& out, DiagnosticCollector& diag);
    void validate_application(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_assets(const YamlNode* node, const std::string& project_dir, Manifest& out, DiagnosticCollector& diag);
    void validate_dependencies(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_build(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_nacp(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_npdm(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_packaging(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void validate_development(const YamlNode* node, Manifest& out, DiagnosticCollector& diag);
    void check_unknown_fields(const YamlNode& node, const std::vector<std::string>& allowed,
                              const std::string& section_prefix, DiagnosticCollector& diag);

    static bool is_valid_title_id(std::string_view tid, uint64_t& out_numeric);
    static bool is_valid_locale_identifier(std::string_view locale);
    static bool verify_jpeg_magic(const std::string& filepath);
};

} // namespace nxdev::manifest
