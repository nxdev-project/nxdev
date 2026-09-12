#include <nxdev/manifest/manifest.hpp>
#include <sstream>
#include <iomanip>

namespace nxdev::manifest {

std::string ApplicationConfig::format_title_id() const {
    if (title_id_numeric.has_value()) {
        std::ostringstream oss;
        oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << *title_id_numeric;
        return oss.str();
    }
    if (title_id.has_value()) {
        return *title_id;
    }
    return "None";
}

static std::string escape_json(std::string_view s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (static_cast<unsigned char>(c) <= 0x1f) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            o << c;
        }
    }
    return o.str();
}

std::string Manifest::to_human_readable() const {
    std::ostringstream oss;
    oss << "=== NXDevAppManifest (schemaVersion: " << schema_version_ << ") ===\n";
    oss << "Project Root:        " << (project_dir_.empty() ? "." : project_dir_) << "\n";
    oss << "Manifest File:       " << (manifest_filepath_.empty() ? "nxapp.yaml" : manifest_filepath_) << "\n\n";

    oss << "Application:\n";
    oss << "  Name:              " << application_.name << "\n";
    oss << "  Author:            " << application_.author << "\n";
    oss << "  Version:           " << application_.version << "\n";
    oss << "  Title ID:          " << application_.format_title_id() << "\n";
    if (!application_.description.empty()) {
        oss << "  Description:       " << application_.description << "\n";
    }
    if (!application_.localized.empty()) {
        oss << "  Localized:\n";
        for (const auto& [loc, info] : application_.localized) {
            oss << "    [" << loc << "] " << info.name << " by " << info.author << "\n";
        }
    }

    oss << "\nAssets:\n";
    oss << "  Icon Source:       " << (assets_.icon.type == IconSourceType::LibnxDefault ? "libnx Default" : assets_.icon.raw_path) << "\n";
    if (assets_.icon.type == IconSourceType::ProjectFile) {
        oss << "  Icon Resolved:     " << assets_.icon.resolved_path << "\n";
    }
    oss << "  RomFS:             " << (assets_.romfs.enabled ? assets_.romfs.raw_path : "None") << "\n";
    if (assets_.romfs.enabled) {
        oss << "  RomFS Resolved:    " << assets_.romfs.resolved_path << "\n";
    }

    oss << "\nDependencies (" << dependencies_.size() << "):\n";
    if (dependencies_.empty()) {
        oss << "  (None declared; nxdev.core implicit)\n";
    } else {
        for (const auto& dep : dependencies_) {
            oss << "  - " << dep.name << " (" << dep.version << ")" << (dep.optional ? " [optional]" : "") << "\n";
        }
    }

    oss << "\nBuild:\n";
    oss << "  Language / Std:    " << build_.language << " (" << build_.standard << ")\n";
    oss << "  Default Profile:   " << build_.default_profile << "\n";
    oss << "  Profiles Config:   " << build_.profiles.size() << " configured\n";

    oss << "\nNACP Configuration:\n";
    oss << "  Display Version:   " << nacp_.display_version << "\n";
    oss << "  Screenshot:        " << (nacp_.screenshot == ScreenshotMode::Allow ? "Allow" : "Deny") << "\n";

    oss << "\nNPDM Configuration:\n";
    oss << "  Preset:            " << static_cast<int>(npdm_.preset) << "\n";
    oss << "  Main Thread Stack: " << (npdm_.main_thread.stack_size / 1024) << " KB\n";

    oss << "\nPackaging:\n";
    oss << "  Default Format:    " << (packaging_.default_format == PackageFormat::NRO ? "NRO" : "NSP") << "\n";
    oss << "  NRO Enabled:       " << (packaging_.nro.enabled ? "true" : "false") << "\n";
    oss << "  NSP Enabled:       " << (packaging_.nsp.enabled ? "true" : "false") << "\n";

    return oss.str();
}

std::string Manifest::to_json() const {
    std::ostringstream j;
    j << "{\n";
    j << "  \"schemaVersion\": " << schema_version_ << ",\n";
    j << "  \"projectDir\": \"" << escape_json(project_dir_) << "\",\n";
    j << "  \"manifestFilepath\": \"" << escape_json(manifest_filepath_) << "\",\n";

    // Application
    j << "  \"application\": {\n";
    j << "    \"name\": \"" << escape_json(application_.name) << "\",\n";
    j << "    \"author\": \"" << escape_json(application_.author) << "\",\n";
    j << "    \"version\": \"" << escape_json(application_.version) << "\",\n";
    if (application_.title_id.has_value()) {
        j << "    \"titleId\": \"" << escape_json(*application_.title_id) << "\",\n";
    }
    if (application_.title_id_numeric.has_value()) {
        j << "    \"titleIdNumeric\": " << *application_.title_id_numeric << ",\n";
    }
    j << "    \"description\": \"" << escape_json(application_.description) << "\",\n";
    j << "    \"localized\": {\n";
    size_t loc_idx = 0;
    for (const auto& [loc, info] : application_.localized) {
        j << "      \"" << escape_json(loc) << "\": {\n";
        j << "        \"name\": \"" << escape_json(info.name) << "\",\n";
        j << "        \"author\": \"" << escape_json(info.author) << "\",\n";
        j << "        \"description\": \"" << escape_json(info.description) << "\"\n";
        j << "      }" << (loc_idx + 1 < application_.localized.size() ? "," : "") << "\n";
        loc_idx++;
    }
    j << "    }\n";
    j << "  },\n";

    // Assets
    j << "  \"assets\": {\n";
    j << "    \"icon\": {\n";
    j << "      \"sourceType\": \"" << (assets_.icon.type == IconSourceType::LibnxDefault ? "libnx_default" : "project_file") << "\",\n";
    j << "      \"rawPath\": \"" << escape_json(assets_.icon.raw_path) << "\",\n";
    j << "      \"resolvedPath\": \"" << escape_json(assets_.icon.resolved_path) << "\"\n";
    j << "    },\n";
    j << "    \"romfs\": {\n";
    j << "      \"enabled\": " << (assets_.romfs.enabled ? "true" : "false") << ",\n";
    j << "      \"rawPath\": \"" << escape_json(assets_.romfs.raw_path) << "\",\n";
    j << "      \"resolvedPath\": \"" << escape_json(assets_.romfs.resolved_path) << "\"\n";
    j << "    }\n";
    j << "  },\n";

    // Dependencies
    j << "  \"dependencies\": [\n";
    for (size_t i = 0; i < dependencies_.size(); ++i) {
        j << "    {\n";
        j << "      \"name\": \"" << escape_json(dependencies_[i].name) << "\",\n";
        j << "      \"version\": \"" << escape_json(dependencies_[i].version) << "\",\n";
        j << "      \"optional\": " << (dependencies_[i].optional ? "true" : "false") << "\n";
        j << "    }" << (i + 1 < dependencies_.size() ? "," : "") << "\n";
    }
    j << "  ],\n";

    // Build
    j << "  \"build\": {\n";
    j << "    \"language\": \"" << escape_json(build_.language) << "\",\n";
    j << "    \"standard\": \"" << escape_json(build_.standard) << "\",\n";
    j << "    \"defaultProfile\": \"" << escape_json(build_.default_profile) << "\"\n";
    j << "  },\n";

    // Packaging
    j << "  \"packaging\": {\n";
    j << "    \"defaultFormat\": \"" << (packaging_.default_format == PackageFormat::NRO ? "nro" : "nsp") << "\",\n";
    j << "    \"nro\": { \"enabled\": " << (packaging_.nro.enabled ? "true" : "false") << " },\n";
    j << "    \"nsp\": { \"enabled\": " << (packaging_.nsp.enabled ? "true" : "false") << " }\n";
    j << "  }\n";

    j << "}\n";
    return j.str();
}

} // namespace nxdev::manifest
