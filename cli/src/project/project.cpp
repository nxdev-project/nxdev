#include <nxdev/project/project.hpp>
#include <nxdev/manifest/parser.hpp>
#include <filesystem>
#include <sstream>

namespace nxdev::project {

namespace fs = std::filesystem;

std::string NXDevProject::info_summary() const {
    std::ostringstream oss;
    oss << "=== NXDev Project Information ===\n";
    oss << "Root Directory:     " << root_path_ << "\n";
    oss << "Manifest File:      " << manifest_path_ << "\n";
    oss << "Build Directory:    " << build_dir_ << "\n";
    if (!local_config_path_.empty() && fs::exists(local_config_path_)) {
        oss << "Local Config:       " << local_config_path_ << "\n";
    }

    if (is_valid_) {
        oss << "\nApplication:\n";
        oss << "  Name:             " << manifest_.application().name << "\n";
        oss << "  Author:           " << manifest_.application().author << "\n";
        oss << "  Version:          " << manifest_.application().version << "\n";
        oss << "  Title ID:         " << manifest_.application().format_title_id() << "\n";
        oss << "  Packaging:        " << (manifest_.packaging().default_format == manifest::PackageFormat::NRO ? "NRO" : "NSP") << "\n";
        oss << "  Dependencies:     " << manifest_.dependencies().size() << " declared\n";
    } else {
        oss << "\nStatus:             INVALID MANIFEST (" << diagnostics_.error_count() << " errors)\n";
    }
    return oss.str();
}

std::string NXDevProject::info_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"root\": \"" << root_path_ << "\",\n";
    oss << "  \"manifestPath\": \"" << manifest_path_ << "\",\n";
    oss << "  \"buildDirectory\": \"" << build_dir_ << "\",\n";
    oss << "  \"isValid\": " << (is_valid_ ? "true" : "false") << ",\n";
    if (is_valid_) {
        oss << "  \"manifest\": " << manifest_.to_json() << "\n";
    } else {
        oss << "  \"manifest\": null,\n";
        oss << "  \"errorCount\": " << diagnostics_.error_count() << "\n";
    }
    oss << "}\n";
    return oss.str();
}

std::optional<NXDevProject> NXDevProject::discover(const std::string& start_dir) {
    manifest::Parser parser;
    auto found_manifest = parser.discover_manifest(start_dir);
    if (!found_manifest.has_value()) {
        return std::nullopt;
    }
    return load_explicit(*found_manifest);
}

std::optional<NXDevProject> NXDevProject::load_explicit(const std::string& path) {
    fs::path p(path);
    if (!fs::exists(p)) {
        return std::nullopt;
    }

    fs::path manifest_path;
    fs::path root_path;

    if (fs::is_directory(p)) {
        manifest_path = p / manifest::Parser::DEFAULT_MANIFEST_FILENAME;
        root_path = p;
        if (!fs::exists(manifest_path)) {
            return std::nullopt;
        }
    } else {
        manifest_path = p;
        root_path = p.parent_path();
        if (root_path.empty()) root_path = fs::current_path();
    }

    root_path = fs::absolute(root_path).lexically_normal();
    manifest_path = fs::absolute(manifest_path).lexically_normal();

    NXDevProject proj;
    proj.root_path_ = root_path.string();
    proj.manifest_path_ = manifest_path.string();
    proj.build_dir_ = (root_path / "build").string();
    proj.local_config_path_ = (root_path / ".nxdev" / "local.yaml").string();

    manifest::Parser parser;
    auto parse_res = parser.parse_file(manifest_path.string());
    proj.diagnostics_ = parse_res.diagnostics;
    proj.is_valid_ = parse_res.has_value();
    if (proj.is_valid_) {
        proj.manifest_ = parse_res.value();
    }

    return proj;
}

} // namespace nxdev::project
