#include <nxdev/pack/pack.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/nsp_backend.hpp>
#include <sstream>
#include <iomanip>

namespace nxdev::pack {

std::string package_format_to_string(PackageFormat format) {
    switch (format) {
        case PackageFormat::NRO: return "nro";
        case PackageFormat::NSP: return "nsp";
    }
    return "unknown";
}

std::optional<PackageFormat> parse_package_format(std::string_view str) {
    if (str == "nro" || str == "NRO") return PackageFormat::NRO;
    if (str == "nsp" || str == "NSP") return PackageFormat::NSP;
    return std::nullopt;
}

std::string package_stage_to_string(PackageStage stage) {
    switch (stage) {
        case PackageStage::Preflight: return "Preflight";
        case PackageStage::Build: return "Build";
        case PackageStage::GenerateNACP: return "Generate NACP";
        case PackageStage::ResolveAssets: return "Resolve assets";
        case PackageStage::CreateNRO: return "Create NRO";
        case PackageStage::PrepareExeFS: return "Prepare ExeFS";
        case PackageStage::GenerateNPDM: return "Generate NPDM";
        case PackageStage::CreateNSP: return "Create NSP";
        case PackageStage::Validate: return "Validate";
        case PackageStage::Finalize: return "Finalize";
    }
    return "Unknown";
}

std::string pack_error_code_to_string(PackErrorCode code) {
    switch (code) {
        case PackErrorCode::None: return "None";
        case PackErrorCode::ToolNotFound: return "ToolNotFound";
        case PackErrorCode::InvalidInputBinary: return "InvalidInputBinary";
        case PackErrorCode::InvalidIcon: return "InvalidIcon";
        case PackErrorCode::InvalidRomFS: return "InvalidRomFS";
        case PackErrorCode::RomfsGenerationFailed: return "RomfsGenerationFailed";
        case PackErrorCode::NacpGenerationFailed: return "NacpGenerationFailed";
        case PackErrorCode::MissingTitleId: return "MissingTitleId";
        case PackErrorCode::InvalidTitleId: return "InvalidTitleId";
        case PackErrorCode::InvalidNpdm: return "InvalidNpdm";
        case PackErrorCode::NsoConversionFailed: return "NsoConversionFailed";
        case PackErrorCode::NpdmGenerationFailed: return "NpdmGenerationFailed";
        case PackErrorCode::KeyFileMissing: return "KeyFileMissing";
        case PackErrorCode::PackagingFailed: return "PackagingFailed";
        case PackErrorCode::ValidationFailed: return "ValidationFailed";
        case PackErrorCode::StaleBuild: return "StaleBuild";
        case PackErrorCode::NotImplemented: return "NotImplemented";
    }
    return "UnknownError";
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

std::string PackageResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"" << (success ? "success" : "error") << "\",\n";
    ss << "  \"format\": \"" << package_format_to_string(format) << "\",\n";
    ss << "  \"profile\": \"" << escape_json_str(profile) << "\",\n";
    ss << "  \"artifact\": \"" << escape_json_str(output_file) << "\",\n";
    ss << "  \"fileSizeBytes\": " << file_size_bytes << ",\n";
    ss << "  \"elf\": \"" << escape_json_str(input_elf_path) << "\",\n";
    if (!backend_name.empty()) {
        ss << "  \"backend\": {\n";
        ss << "    \"name\": \"" << escape_json_str(backend_name) << "\",\n";
        ss << "    \"revision\": \"" << escape_json_str(backend_version) << "\"\n";
        ss << "  },\n";
    }
    ss << "  \"metadata\": {\n";
    if (!title_id.empty()) {
        ss << "    \"titleId\": \"" << escape_json_str(title_id) << "\",\n";
    }
    if (!npdm_file.empty()) {
        ss << "    \"npdm\": \"" << escape_json_str(npdm_file) << "\",\n";
    }
    if (!nso_file.empty()) {
        ss << "    \"nso\": \"" << escape_json_str(nso_file) << "\",\n";
    }
    ss << "    \"nacp\": \"" << escape_json_str(nacp_file) << "\",\n";
    ss << "    \"icon\": \"" << escape_json_str(icon_file) << "\",\n";
    ss << "    \"iconSource\": \"" << escape_json_str(icon_source) << "\",\n";
    ss << "    \"romfs\": \"" << escape_json_str(romfs_dir) << "\",\n";
    ss << "    \"romfsStaged\": {\n";
    ss << "      \"staged\": " << (!romfs_dir.empty() ? "true" : "false") << ",\n";
    ss << "      \"path\": \"" << escape_json_str(romfs_dir) << "\",\n";
    ss << "      \"manifest\": \"" << escape_json_str(romfs_manifest_file) << "\",\n";
    ss << "      \"fingerprint\": \"" << escape_json_str(romfs_fingerprint) << "\",\n";
    ss << "      \"filesCount\": " << romfs_files_count << ",\n";
    ss << "      \"overridesCount\": " << romfs_overrides_count << ",\n";
    ss << "      \"layers\": [";
    for (size_t i = 0; i < romfs_layers.size(); ++i) {
        ss << "\"" << escape_json_str(romfs_layers[i]) << "\"" << (i + 1 < romfs_layers.size() ? ", " : "");
    }
    ss << "]\n";
    ss << "    }\n";
    ss << "  },\n";
    if (!success) {
        ss << "  \"error\": {\n";
        ss << "    \"code\": \"" << pack_error_code_to_string(error_code) << "\",\n";
        if (backend_info.has_value() && !backend_info->stage.empty()) {
            ss << "    \"stage\": \"" << escape_json_str(backend_info->stage) << "\",\n";
        }
        ss << "    \"message\": \"" << escape_json_str(error_message) << "\",\n";
        ss << "    \"exitCode\": " << exit_code;
        if (backend_info.has_value()) {
            ss << ",\n    \"backend\": {\n";
            ss << "      \"name\": \"" << escape_json_str(backend_info->name) << "\",\n";
            ss << "      \"revision\": \"" << escape_json_str(backend_info->revision) << "\",\n";
            ss << "      \"executable\": \"" << escape_json_str(backend_info->executable) << "\",\n";
            ss << "      \"workingDir\": \"" << escape_json_str(backend_info->working_dir) << "\",\n";
            ss << "      \"logPath\": \"" << escape_json_str(backend_info->log_path) << "\",\n";
            ss << "      \"stagingDir\": \"" << escape_json_str(backend_info->staging_dir) << "\",\n";
            ss << "      \"exitCode\": " << backend_info->exit_code << ",\n";
            ss << "      \"stdout\": \"" << escape_json_str(backend_info->stdout_output) << "\",\n";
            ss << "      \"stderr\": \"" << escape_json_str(backend_info->stderr_output) << "\"\n";
            ss << "    }\n";
        } else {
            ss << "\n";
        }
        ss << "  },\n";
    }
    ss << "  \"logs\": [";
    for (size_t i = 0; i < log_messages.size(); ++i) {
        ss << "\"" << escape_json_str(log_messages[i]) << "\"" << (i + 1 < log_messages.size() ? ", " : "");
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

PackManager::PackManager() {
    register_backend(std::make_unique<NroPackBackend>());
    register_backend(std::make_unique<NspPackBackend>());
}

void PackManager::register_backend(std::unique_ptr<IPackBackend> backend) {
    if (backend) {
        backends_[backend->get_format()] = std::move(backend);
    }
}

IPackBackend* PackManager::get_backend(PackageFormat format) const noexcept {
    auto it = backends_.find(format);
    if (it != backends_.end()) {
        return it->second.get();
    }
    return nullptr;
}

PackageResult PackManager::pack(
    const PackageRequest& request,
    const env::Environment* env,
    ProgressCallback progress
) {
    auto* backend = get_backend(request.format);
    if (!backend) {
        PackageResult res;
        res.success = false;
        res.format = request.format;
        res.profile = request.profile;
        res.error_code = PackErrorCode::ToolNotFound;
        res.error_message = "No packaging backend registered for requested format: " + package_format_to_string(request.format);
        return res;
    }

    return backend->pack(request, env, progress);
}

PackOperationResult PackManager::pack(
    const manifest::Manifest& manifest,
    const PackOptions& options
) {
    PackageRequest req = options;
    req.manifest = manifest;
    auto res = pack(req, nullptr, nullptr);
    return PackOperationResult{
        .success = res.success,
        .result = res
    };
}

} // namespace nxdev::pack
