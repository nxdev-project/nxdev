#include <nxdev/manifest/validator.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <set>

namespace nxdev::manifest {

namespace fs = std::filesystem;

static std::string to_lower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

std::optional<Manifest> ManifestValidator::validate_and_normalize(
    const YamlNode& root_node,
    const std::string& manifest_filepath,
    const std::string& project_dir,
    DiagnosticCollector& diagnostics
) {
    if (!root_node.is_mapping()) {
        diagnostics.add_error(DiagnosticCode::YamlSyntaxError,
                              "Root of nxapp.yaml manifest must be a YAML mapping (key-value dictionary)",
                              "", root_node.location(), manifest_filepath);
        return std::nullopt;
    }

    Manifest manifest;
    manifest.set_manifest_filepath(manifest_filepath);
    manifest.set_project_dir(project_dir);

    // 1. Check unknown top-level keys
    static const std::vector<std::string> ALLOWED_TOP_LEVEL = {
        "schemaVersion", "application", "assets", "dependencies", "build",
        "nacp", "npdm", "packaging", "development"
    };
    check_unknown_fields(root_node, ALLOWED_TOP_LEVEL, "", diagnostics);

    // 2. Schema version
    validate_schema_version(root_node, manifest, diagnostics);

    // 3. Application section (required)
    const auto* app_node = root_node.get("application");
    if (!app_node) {
        diagnostics.add_error(DiagnosticCode::MissingRequiredField,
                              "Manifest is missing required 'application' section",
                              "application", root_node.location(), manifest_filepath);
    } else {
        validate_application(app_node, manifest, diagnostics);
    }

    // 4. Assets section
    validate_assets(root_node.get("assets"), project_dir, manifest, diagnostics);

    // 5. Dependencies section
    validate_dependencies(root_node.get("dependencies"), manifest, diagnostics);

    // 6. Build section
    validate_build(root_node.get("build"), manifest, diagnostics);

    // 7. NACP section
    validate_nacp(root_node.get("nacp"), manifest, diagnostics);

    // 8. NPDM section
    validate_npdm(root_node.get("npdm"), manifest, diagnostics);

    // 9. Packaging section
    validate_packaging(root_node.get("packaging"), manifest, diagnostics);

    // 10. Development section
    validate_development(root_node.get("development"), manifest, diagnostics);

    if (diagnostics.has_errors()) {
        return std::nullopt;
    }

    return manifest;
}

void ManifestValidator::check_unknown_fields(
    const YamlNode& node,
    const std::vector<std::string>& allowed,
    const std::string& section_prefix,
    DiagnosticCollector& diag
) {
    if (!node.is_mapping()) return;

    for (const auto& [key, child] : node.as_mapping()) {
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            std::string field_path = section_prefix.empty() ? key : (section_prefix + "." + key);
            diag.add_error(DiagnosticCode::UnknownField,
                           "Unknown or unexpected field '" + field_path + "' in manifest",
                           field_path, child.location());
        }
    }
}

void ManifestValidator::validate_schema_version(
    const YamlNode& root,
    Manifest& out,
    DiagnosticCollector& diag
) {
    const auto* node = root.get("schemaVersion");
    if (!node) {
        diag.add_error(DiagnosticCode::MissingRequiredField,
                       "Missing required 'schemaVersion' field (expected 'schemaVersion: 1')",
                       "schemaVersion", root.location());
        return;
    }

    auto ver = node->as_uint();
    if (!ver.has_value()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'schemaVersion' must be a positive integer",
                       "schemaVersion", node->location());
        return;
    }

    if (*ver != Manifest::SUPPORTED_SCHEMA_VERSION) {
        diag.add_error(DiagnosticCode::UnsupportedSchemaVersion,
                       "Unsupported schemaVersion '" + std::to_string(*ver) + "'. Current supported version is 1",
                       "schemaVersion", node->location());
        return;
    }

    out.set_schema_version(static_cast<uint32_t>(*ver));
}

void ManifestValidator::validate_application(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'application' must be a YAML mapping object",
                       "application", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_APP_FIELDS = {
        "name", "author", "version", "titleId", "description", "localized"
    };
    check_unknown_fields(*node, ALLOWED_APP_FIELDS, "application", diag);

    // name (required)
    const auto* name_node = node->get("name");
    if (!name_node || name_node->as_string().empty()) {
        diag.add_error(DiagnosticCode::MissingRequiredField,
                       "'application.name' is required and must not be empty",
                       "application.name", node->location());
    } else {
        if (name_node->as_string().size() > 255) {
            diag.add_error(DiagnosticCode::InvalidFieldType,
                           "'application.name' exceeds maximum length of 255 characters",
                           "application.name", name_node->location());
        }
        out.application().name = name_node->as_string();
    }

    // author (required)
    const auto* author_node = node->get("author");
    if (!author_node || author_node->as_string().empty()) {
        diag.add_error(DiagnosticCode::MissingRequiredField,
                       "'application.author' is required and must not be empty",
                       "application.author", node->location());
    } else {
        if (author_node->as_string().size() > 255) {
            diag.add_error(DiagnosticCode::InvalidFieldType,
                           "'application.author' exceeds maximum length of 255 characters",
                           "application.author", author_node->location());
        }
        out.application().author = author_node->as_string();
    }

    // version (optional, default: "1.0.0")
    const auto* ver_node = node->get("version");
    if (ver_node && !ver_node->as_string().empty()) {
        out.application().version = ver_node->as_string();
    } else {
        out.application().version = "1.0.0";
    }

    // titleId (optional)
    const auto* tid_node = node->get("titleId");
    if (tid_node && !tid_node->as_string().empty()) {
        std::string tid_str = tid_node->as_string();
        uint64_t numeric_val = 0;
        if (!is_valid_title_id(tid_str, numeric_val)) {
            diag.add_error(DiagnosticCode::InvalidTitleId,
                           "Invalid 'application.titleId' '" + tid_str + "': must be 16 hexadecimal digits (e.g. '0100000000001000')",
                           "application.titleId", tid_node->location());
        } else {
            out.application().title_id = tid_str;
            out.application().title_id_numeric = numeric_val;
        }
    }

    // description (optional)
    const auto* desc_node = node->get("description");
    if (desc_node) {
        out.application().description = desc_node->as_string();
    }

    // localized (optional)
    const auto* loc_node = node->get("localized");
    if (loc_node) {
        if (!loc_node->is_mapping()) {
            diag.add_error(DiagnosticCode::InvalidFieldType,
                           "'application.localized' must be a mapping of locale codes to title info",
                           "application.localized", loc_node->location());
        } else {
            for (const auto& [loc_key, loc_entry] : loc_node->as_mapping()) {
                if (!is_valid_locale_identifier(loc_key)) {
                    diag.add_error(DiagnosticCode::InvalidLocaleCode,
                                   "Unsupported locale code '" + loc_key + "'. Use BCP-47 or Switch language code (e.g. 'en-US', 'ja', 'pt-BR')",
                                   "application.localized." + loc_key, loc_entry.location());
                    continue;
                }
                if (!loc_entry.is_mapping()) {
                    diag.add_error(DiagnosticCode::InvalidFieldType,
                                   "Localized info for '" + loc_key + "' must be a mapping object with 'name' and 'author'",
                                   "application.localized." + loc_key, loc_entry.location());
                    continue;
                }
                LocalizedAppInfo loc_info;
                if (const auto* n = loc_entry.get("name")) loc_info.name = n->as_string();
                if (const auto* a = loc_entry.get("author")) loc_info.author = a->as_string();
                if (const auto* d = loc_entry.get("description")) loc_info.description = d->as_string();

                if (loc_info.name.empty()) {
                    loc_info.name = out.application().name;
                }
                if (loc_info.author.empty()) {
                    loc_info.author = out.application().author;
                }
                out.application().localized[loc_key] = std::move(loc_info);
            }
        }
    }
}

void ManifestValidator::validate_assets(
    const YamlNode* node,
    const std::string& project_dir,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) {
        // Defaults: LibnxDefault icon, RomFS disabled
        out.assets().icon.type = IconSourceType::LibnxDefault;
        out.assets().romfs.enabled = false;
        return;
    }

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'assets' must be a YAML mapping object",
                       "assets", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_ASSET_FIELDS = {"icon", "romfs"};
    check_unknown_fields(*node, ALLOWED_ASSET_FIELDS, "assets", diag);

    // icon
    const auto* icon_node = node->get("icon");
    if (icon_node && !icon_node->as_string().empty()) {
        std::string raw_path = icon_node->as_string();
        fs::path base = project_dir.empty() ? fs::current_path() : fs::path(project_dir);
        fs::path resolved = (fs::path(raw_path).is_absolute()) ? fs::path(raw_path) : (base / raw_path);
        resolved = resolved.lexically_normal();

        // Check extension
        std::string ext = to_lower(resolved.extension().string());
        if (ext != ".jpg" && ext != ".jpeg") {
            diag.add_error(DiagnosticCode::InvalidAssetFormat,
                           "Application icon must be a JPEG image (.jpg or .jpeg), found: " + raw_path,
                           "assets.icon", icon_node->location());
        } else if (!fs::exists(resolved)) {
            diag.add_error(DiagnosticCode::AssetNotFound,
                           "Specified icon file does not exist: " + resolved.string(),
                           "assets.icon", icon_node->location());
        } else if (!verify_jpeg_magic(resolved.string())) {
            diag.add_error(DiagnosticCode::InvalidAssetFormat,
                           "Specified icon is not a valid JPEG file (invalid file header): " + raw_path,
                           "assets.icon", icon_node->location());
        } else {
            out.assets().icon.type = IconSourceType::ProjectFile;
            out.assets().icon.raw_path = raw_path;
            out.assets().icon.resolved_path = resolved.string();
        }
    } else {
        out.assets().icon.type = IconSourceType::LibnxDefault;
    }

    // romfs
    const auto* romfs_node = node->get("romfs");
    if (romfs_node && !romfs_node->as_string().empty()) {
        std::string raw_path = romfs_node->as_string();
        fs::path base = project_dir.empty() ? fs::current_path() : fs::path(project_dir);
        fs::path resolved = (fs::path(raw_path).is_absolute()) ? fs::path(raw_path) : (base / raw_path);
        resolved = resolved.lexically_normal();

        if (!fs::exists(resolved)) {
            diag.add_error(DiagnosticCode::AssetNotFound,
                           "Specified RomFS directory does not exist: " + resolved.string(),
                           "assets.romfs", romfs_node->location());
        } else if (!fs::is_directory(resolved)) {
            diag.add_error(DiagnosticCode::InvalidRomFsPath,
                           "Specified RomFS path must be a directory, found regular file: " + raw_path,
                           "assets.romfs", romfs_node->location());
        } else {
            out.assets().romfs.enabled = true;
            out.assets().romfs.raw_path = raw_path;
            out.assets().romfs.resolved_path = resolved.string();
        }
    } else {
        out.assets().romfs.enabled = false;
    }
}

void ManifestValidator::validate_dependencies(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) return;

    if (!node->is_sequence()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'dependencies' must be a list",
                       "dependencies", node->location());
        return;
    }

    std::set<std::string> seen_names;

    for (const auto& item : node->as_sequence()) {
        Dependency dep;
        if (item.is_scalar()) {
            dep.name = item.as_string();
            dep.version = "*";
            dep.optional = false;
        } else if (item.is_mapping()) {
            const auto* name_node = item.get("name");
            if (!name_node || name_node->as_string().empty()) {
                diag.add_error(DiagnosticCode::MissingRequiredField,
                               "Dependency item is missing required 'name' field",
                               "dependencies", item.location());
                continue;
            }
            dep.name = name_node->as_string();
            if (const auto* v = item.get("version")) dep.version = v->as_string();
            if (const auto* opt = item.get("optional")) dep.optional = opt->as_bool().value_or(false);
        } else {
            diag.add_error(DiagnosticCode::InvalidFieldType,
                           "Dependency item must be a string name or mapping object",
                           "dependencies", item.location());
            continue;
        }

        if (seen_names.count(dep.name) > 0) {
            diag.add_error(DiagnosticCode::DuplicateDependency,
                           "Duplicate dependency declaration detected: '" + dep.name + "'",
                           "dependencies." + dep.name, item.location());
        } else {
            seen_names.insert(dep.name);
            out.dependencies().push_back(std::move(dep));
        }
    }
}

void ManifestValidator::validate_build(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) return;

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'build' must be a mapping object",
                       "build", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_BUILD = {
        "language", "standard", "sources", "includes", "defines", "defaultProfile", "profiles"
    };
    check_unknown_fields(*node, ALLOWED_BUILD, "build", diag);

    if (const auto* l = node->get("language")) out.build().language = l->as_string();
    if (const auto* s = node->get("standard")) out.build().standard = s->as_string();
    if (const auto* dp = node->get("defaultProfile")) out.build().default_profile = dp->as_string();

    // Parse profiles
    if (const auto* prof_node = node->get("profiles")) {
        if (prof_node->is_mapping()) {
            for (const auto& [pname, pval] : prof_node->as_mapping()) {
                BuildProfile prof;
                if (pval.is_mapping()) {
                    if (const auto* opt = pval.get("optimization")) {
                        std::string opt_str = to_lower(opt->as_string());
                        if (opt_str == "debug") prof.optimization = OptimizationLevel::Debug;
                        else if (opt_str == "release") prof.optimization = OptimizationLevel::Release;
                        else if (opt_str == "size") prof.optimization = OptimizationLevel::Size;
                        else if (opt_str == "fast") prof.optimization = OptimizationLevel::Fast;
                    }
                    if (const auto* sym = pval.get("symbols")) {
                        prof.symbols = sym->as_bool().value_or(true);
                    }
                }
                out.build().profiles[pname] = std::move(prof);
            }
        }
    }
}

void ManifestValidator::validate_nacp(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    // Default display_version to application.version
    out.nacp().display_version = out.application().version;

    if (!node) return;

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'nacp' must be a mapping object",
                       "nacp", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_NACP = {
        "displayVersion", "startupUserAccount", "userAccountSwitchLock",
        "screenshot", "videoCapture", "logoType", "parentalControl"
    };
    check_unknown_fields(*node, ALLOWED_NACP, "nacp", diag);

    if (const auto* dv = node->get("displayVersion")) {
        out.nacp().display_version = dv->as_string();
    }
    if (const auto* sua = node->get("startupUserAccount")) {
        std::string s = to_lower(sua->as_string());
        if (s == "none") out.nacp().startup_user_account = StartupUserAccount::None;
        else if (s == "required") out.nacp().startup_user_account = StartupUserAccount::Required;
        else if (s == "optional") out.nacp().startup_user_account = StartupUserAccount::Optional;
        else diag.add_error(DiagnosticCode::InvalidNacpConfig, "Invalid 'startupUserAccount': must be 'none', 'required', or 'optional'", "nacp.startupUserAccount", sua->location());
    }
    if (const auto* scr = node->get("screenshot")) {
        std::string s = to_lower(scr->as_string());
        if (s == "allow") out.nacp().screenshot = ScreenshotMode::Allow;
        else if (s == "deny") out.nacp().screenshot = ScreenshotMode::Deny;
        else diag.add_error(DiagnosticCode::InvalidNacpConfig, "Invalid 'screenshot': must be 'allow' or 'deny'", "nacp.screenshot", scr->location());
    }
    if (const auto* vc = node->get("videoCapture")) {
        std::string s = to_lower(vc->as_string());
        if (s == "disabled") out.nacp().video_capture = VideoCaptureMode::Disabled;
        else if (s == "manual") out.nacp().video_capture = VideoCaptureMode::Manual;
        else if (s == "automatic") out.nacp().video_capture = VideoCaptureMode::Automatic;
        else diag.add_error(DiagnosticCode::InvalidNacpConfig, "Invalid 'videoCapture': must be 'disabled', 'manual', or 'automatic'", "nacp.videoCapture", vc->location());
    }
}

void ManifestValidator::validate_npdm(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) return;

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'npdm' must be a mapping object",
                       "npdm", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_NPDM = {
        "preset", "mainThread", "addressSpace", "services", "filesystem", "kernelCapabilities"
    };
    check_unknown_fields(*node, ALLOWED_NPDM, "npdm", diag);

    if (const auto* p = node->get("preset")) {
        std::string s = to_lower(p->as_string());
        if (s == "standard") out.npdm().preset = NpdmPreset::Standard;
        else if (s == "network") out.npdm().preset = NpdmPreset::Network;
        else if (s == "filesystem") out.npdm().preset = NpdmPreset::Filesystem;
        else if (s == "multimedia") out.npdm().preset = NpdmPreset::Multimedia;
        else if (s == "advanced") out.npdm().preset = NpdmPreset::Advanced;
        else diag.add_error(DiagnosticCode::InvalidNpdmConfig, "Invalid NPDM preset '" + p->as_string() + "'. Allowed: standard, network, filesystem, multimedia, advanced", "npdm.preset", p->location());
    }

    if (const auto* mt = node->get("mainThread")) {
        if (mt->is_mapping()) {
            if (const auto* pr = mt->get("priority")) out.npdm().main_thread.priority = static_cast<int32_t>(pr->as_int().value_or(44));
            if (const auto* co = mt->get("core")) out.npdm().main_thread.core = static_cast<int32_t>(co->as_int().value_or(0));
            if (const auto* st = mt->get("stackSize")) out.npdm().main_thread.stack_size = static_cast<uint32_t>(st->as_uint().value_or(0x40000));
        }
    }
}

void ManifestValidator::validate_packaging(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) return;

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'packaging' must be a mapping object",
                       "packaging", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_PACK = {"defaultFormat", "nro", "nsp"};
    check_unknown_fields(*node, ALLOWED_PACK, "packaging", diag);

    if (const auto* df = node->get("defaultFormat")) {
        std::string s = to_lower(df->as_string());
        if (s == "nro") out.packaging().default_format = PackageFormat::NRO;
        else if (s == "nsp") out.packaging().default_format = PackageFormat::NSP;
        else diag.add_error(DiagnosticCode::InvalidPackagingConfig, "Invalid 'defaultFormat': must be 'nro' or 'nsp'", "packaging.defaultFormat", df->location());
    }
}

void ManifestValidator::validate_development(
    const YamlNode* node,
    Manifest& out,
    DiagnosticCollector& diag
) {
    if (!node) return;

    if (!node->is_mapping()) {
        diag.add_error(DiagnosticCode::InvalidFieldType,
                       "'development' must be a mapping object",
                       "development", node->location());
        return;
    }

    static const std::vector<std::string> ALLOWED_DEV = {"nxlink"};
    check_unknown_fields(*node, ALLOWED_DEV, "development", diag);

    if (const auto* nxlink = node->get("nxlink")) {
        if (nxlink->is_mapping()) {
            if (const auto* h = nxlink->get("host")) out.development().nxlink.host = h->as_string();
            if (const auto* p = nxlink->get("port")) out.development().nxlink.port = static_cast<uint16_t>(p->as_uint().value_or(28771));
        }
    }
}

bool ManifestValidator::is_valid_title_id(std::string_view tid, uint64_t& out_numeric) {
    std::string s(tid);
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() != 16) return false;

    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    try {
        out_numeric = std::stoull(s, nullptr, 16);
        return true;
    } catch (...) {
        return false;
    }
}

bool ManifestValidator::is_valid_locale_identifier(std::string_view locale) {
    static const std::set<std::string> VALID_LOCALES = {
        "en-US", "en-GB", "ja", "fr", "fr-CA", "de", "es", "es-419",
        "it", "nl", "pt", "pt-BR", "ru", "zh-Hans", "zh-Hant", "ko"
    };
    return VALID_LOCALES.count(std::string(locale)) > 0;
}

bool ManifestValidator::verify_jpeg_magic(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    unsigned char header[3];
    if (!file.read(reinterpret_cast<char*>(header), 3)) return false;

    return (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF);
}

} // namespace nxdev::manifest
