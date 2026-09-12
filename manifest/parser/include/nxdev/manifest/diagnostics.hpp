#pragma once

#include <nxdev/manifest/yaml.hpp>
#include <string>
#include <vector>
#include <string_view>
#include <iostream>

namespace nxdev::manifest {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Info,
    Note
};

namespace DiagnosticCode {
    inline constexpr std::string_view FileNotFound = "NXM001";
    inline constexpr std::string_view YamlSyntaxError = "NXM002";
    inline constexpr std::string_view MissingRequiredField = "NXM003";
    inline constexpr std::string_view UnsupportedSchemaVersion = "NXM004";
    inline constexpr std::string_view InvalidFieldType = "NXM005";
    inline constexpr std::string_view UnknownField = "NXM006";
    inline constexpr std::string_view InvalidTitleId = "NXM007";
    inline constexpr std::string_view AssetNotFound = "NXM008";
    inline constexpr std::string_view InvalidAssetFormat = "NXM009";
    inline constexpr std::string_view InvalidRomFsPath = "NXM010";
    inline constexpr std::string_view DuplicateDependency = "NXM011";
    inline constexpr std::string_view InvalidLocaleCode = "NXM012";
    inline constexpr std::string_view InvalidBuildConfig = "NXM013";
    inline constexpr std::string_view InvalidNacpConfig = "NXM014";
    inline constexpr std::string_view InvalidNpdmConfig = "NXM015";
    inline constexpr std::string_view InvalidPackagingConfig = "NXM016";
}

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string code;
    std::string message;
    std::string field_path;
    std::string filename;
    SourceLocation location{1, 1};

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::string severity_string() const noexcept;
};

class DiagnosticCollector {
public:
    DiagnosticCollector() = default;
    ~DiagnosticCollector() = default;

    void add(Diagnostic diag);
    void add_error(std::string_view code, std::string message, std::string field_path = "",
                   SourceLocation loc = {1, 1}, std::string filename = "");
    void add_warning(std::string_view code, std::string message, std::string field_path = "",
                     SourceLocation loc = {1, 1}, std::string filename = "");
    void add_info(std::string_view code, std::string message, std::string field_path = "",
                  SourceLocation loc = {1, 1}, std::string filename = "");

    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] bool has_warnings() const noexcept;
    [[nodiscard]] size_t error_count() const noexcept { return error_count_; }
    [[nodiscard]] size_t warning_count() const noexcept { return warning_count_; }
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const noexcept { return diagnostics_; }

    void print_summary(std::ostream& os) const;
    void clear();

private:
    std::vector<Diagnostic> diagnostics_;
    size_t error_count_{0};
    size_t warning_count_{0};
};

} // namespace nxdev::manifest
