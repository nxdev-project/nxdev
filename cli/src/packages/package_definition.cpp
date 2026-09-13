#include <nxdev/packages/package_definition.hpp>
#include <sstream>

namespace nxdev::packages {

std::string package_kind_to_string(PackageKind kind) {
    switch (kind) {
        case PackageKind::Builtin: return "builtin";
        case PackageKind::DevkitPro: return "devkitpro";
        case PackageKind::Meta: return "meta";
    }
    return "unknown";
}

std::optional<PackageKind> parse_package_kind(std::string_view str) {
    if (str == "builtin") return PackageKind::Builtin;
    if (str == "devkitpro") return PackageKind::DevkitPro;
    if (str == "meta") return PackageKind::Meta;
    return std::nullopt;
}

static std::string escape_json_str(std::string_view s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

std::string PackageDefinition::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"id\": \"" << escape_json_str(id) << "\",\n";
    ss << "  \"name\": \"" << escape_json_str(name) << "\",\n";
    ss << "  \"description\": \"" << escape_json_str(description) << "\",\n";
    ss << "  \"kind\": \"" << package_kind_to_string(kind) << "\",\n";
    ss << "  \"category\": \"" << escape_json_str(category) << "\",\n";
    
    ss << "  \"dependencies\": [";
    for (size_t i = 0; i < dependencies.size(); ++i) {
        ss << "\"" << escape_json_str(dependencies[i]) << "\"" << (i + 1 < dependencies.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"devkitpro\": {\n";
    ss << "    \"packages\": [";
    for (size_t i = 0; i < devkitpro.packages.size(); ++i) {
        ss << "\"" << escape_json_str(devkitpro.packages[i]) << "\"" << (i + 1 < devkitpro.packages.size() ? ", " : "");
    }
    ss << "]\n  },\n";

    ss << "  \"cmake\": {\n";
    ss << "    \"targets\": [";
    for (size_t i = 0; i < cmake.targets.size(); ++i) {
        ss << "\"" << escape_json_str(cmake.targets[i]) << "\"" << (i + 1 < cmake.targets.size() ? ", " : "");
    }
    ss << "],\n";
    ss << "    \"headers\": [";
    for (size_t i = 0; i < cmake.headers.size(); ++i) {
        ss << "\"" << escape_json_str(cmake.headers[i]) << "\"" << (i + 1 < cmake.headers.size() ? ", " : "");
    }
    ss << "],\n";
    ss << "    \"libraries\": [";
    for (size_t i = 0; i < cmake.libraries.size(); ++i) {
        ss << "\"" << escape_json_str(cmake.libraries[i]) << "\"" << (i + 1 < cmake.libraries.size() ? ", " : "");
    }
    ss << "]\n  },\n";

    ss << "  \"license\": \"" << escape_json_str(license) << "\",\n";
    ss << "  \"upstream_url\": \"" << escape_json_str(upstream_url) << "\"\n";
    ss << "}";
    return ss.str();
}

} // namespace nxdev::packages
