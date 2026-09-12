#include <nxdev/manifest/parser.hpp>
#include <nxdev/manifest/validator.hpp>
#include <nxdev/manifest/yaml.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace nxdev::manifest {

namespace fs = std::filesystem;

ParseResult Parser::parse_file(const std::string& filepath) {
    ParseResult result;
    fs::path p(filepath);

    if (!fs::exists(p)) {
        result.diagnostics.add_error(DiagnosticCode::FileNotFound,
                                     "Manifest file not found: " + filepath,
                                     "", {1, 1}, filepath);
        result.success = false;
        return result;
    }

    std::ifstream file(p);
    if (!file.is_open()) {
        result.diagnostics.add_error(DiagnosticCode::FileNotFound,
                                     "Could not open manifest file for reading: " + filepath,
                                     "", {1, 1}, filepath);
        result.success = false;
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string project_dir = p.parent_path().string();
    if (project_dir.empty()) {
        project_dir = ".";
    }

    YamlParseError yaml_err;
    YamlParser yaml_parser;
    auto ast = yaml_parser.parse(buffer.str(), yaml_err);

    if (!ast.has_value()) {
        result.diagnostics.add_error(DiagnosticCode::YamlSyntaxError,
                                     "YAML syntax error: " + yaml_err.message,
                                     "", yaml_err.location, filepath);
        result.success = false;
        return result;
    }

    ManifestValidator validator;
    auto manifest = validator.validate_and_normalize(*ast, filepath, project_dir, result.diagnostics);

    result.success = !result.diagnostics.has_errors() && manifest.has_value();
    result.manifest = std::move(manifest);
    return result;
}

ParseResult Parser::parse_string(const std::string& content, const std::string& project_dir) {
    ParseResult result;

    if (content.empty()) {
        result.diagnostics.add_error(DiagnosticCode::YamlSyntaxError,
                                     "Manifest content is empty",
                                     "", {1, 1});
        result.success = false;
        return result;
    }

    YamlParseError yaml_err;
    YamlParser yaml_parser;
    auto ast = yaml_parser.parse(content, yaml_err);

    if (!ast.has_value()) {
        result.diagnostics.add_error(DiagnosticCode::YamlSyntaxError,
                                     "YAML syntax error: " + yaml_err.message,
                                     "", yaml_err.location);
        result.success = false;
        return result;
    }

    ManifestValidator validator;
    std::string proj = project_dir.empty() ? "." : project_dir;
    auto manifest = validator.validate_and_normalize(*ast, "memory", proj, result.diagnostics);

    result.success = !result.diagnostics.has_errors() && manifest.has_value();
    result.manifest = std::move(manifest);
    return result;
}

std::optional<std::string> Parser::discover_manifest(const std::string& start_dir) {
    fs::path current = start_dir.empty() ? fs::current_path() : fs::path(start_dir);
    current = fs::absolute(current).lexically_normal();

    size_t depth_safety = 0;
    while (depth_safety < 32) {
        fs::path candidate = current / DEFAULT_MANIFEST_FILENAME;
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            return candidate.string();
        }

        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
        depth_safety++;
    }

    return std::nullopt;
}

ParseResult Parser::load_from_discovery(const std::string& start_dir) {
    auto path = discover_manifest(start_dir);
    if (!path.has_value()) {
        ParseResult res;
        res.diagnostics.add_error(DiagnosticCode::FileNotFound,
                                  "No nxapp.yaml found in current working directory or any parent directories",
                                  "", {1, 1});
        res.success = false;
        return res;
    }
    return parse_file(*path);
}

} // namespace nxdev::manifest
