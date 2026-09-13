#pragma once

#include <string>
#include <vector>
#include <optional>
#include <string_view>

namespace nxdev::packages {

enum class PackageKind {
    Builtin,
    DevkitPro,
    Meta
};

std::string package_kind_to_string(PackageKind kind);
std::optional<PackageKind> parse_package_kind(std::string_view str);

struct CMakeIntegration {
    std::vector<std::string> targets;
    std::vector<std::string> headers;
    std::vector<std::string> libraries;
};

struct DevkitProIntegration {
    std::vector<std::string> packages;
};

struct PackageDefinition {
    std::string id;              // e.g. "nxdev.sdl2"
    std::string name;            // e.g. "SDL2"
    std::string description;
    PackageKind kind{PackageKind::DevkitPro};
    std::string category{"general"};
    std::vector<std::string> dependencies; // NXDev module IDs (e.g. ["nxdev.core"])
    DevkitProIntegration devkitpro;
    CMakeIntegration cmake;
    std::string license;
    std::string upstream_url;

    [[nodiscard]] bool is_builtin() const noexcept { return kind == PackageKind::Builtin; }
    [[nodiscard]] bool is_devkitpro() const noexcept { return kind == PackageKind::DevkitPro; }
    [[nodiscard]] bool is_meta() const noexcept { return kind == PackageKind::Meta; }

    [[nodiscard]] std::string to_json() const;
};

} // namespace nxdev::packages
