#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/process.hpp>
#include <nxdev/pack/romfs_stager.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

namespace nxdev::pack {

std::string NroPackBackend::sanitize_filename(std::string_view name) {
    std::string safe;
    safe.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            safe.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (c == ' ' || c == '.' || c == '/') {
            if (!safe.empty() && safe.back() != '-') {
                safe.push_back('-');
            }
        }
    }
    while (!safe.empty() && safe.back() == '-') {
        safe.pop_back();
    }
    while (!safe.empty() && safe.front() == '-') {
        safe.erase(safe.begin());
    }
    return safe.empty() ? "app" : safe;
}

bool NroPackBackend::validate_elf_binary(const std::string& elf_path, std::string& error_message) {
    if (!fs::exists(elf_path)) {
        error_message = "Input ELF binary does not exist at: " + elf_path;
        return false;
    }
    if (!fs::is_regular_file(elf_path)) {
        error_message = "Input ELF path is not a regular file: " + elf_path;
        return false;
    }

    std::ifstream file(elf_path, std::ios::binary);
    if (!file.is_open()) {
        error_message = "Unable to open input ELF binary for reading: " + elf_path;
        return false;
    }

    unsigned char header[20] = {0};
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    std::streamsize read_bytes = file.gcount();

    if (read_bytes < 20) {
        error_message = "Input file is too small to be a valid ELF binary (" + std::to_string(read_bytes) + " bytes)";
        return false;
    }

    // Check ELF magic: 0x7F, 'E', 'L', 'F'
    if (header[0] != 0x7F || header[1] != 'E' || header[2] != 'L' || header[3] != 'F') {
        error_message = "File does not have a valid ELF magic header: " + elf_path;
        return false;
    }

    // Check 64-bit ELF (EI_CLASS == 2)
    if (header[4] != 2) {
        error_message = "ELF binary is not 64-bit (EI_CLASS is not ELFCLASS64)";
        return false;
    }

    // Check AArch64 machine architecture (e_machine at offset 18-19 == 183 / 0x00B7)
    uint16_t machine = static_cast<uint16_t>(header[18] | (header[19] << 8));
    if (machine != 183) { // 183 == EM_AARCH64
        error_message = "ELF binary target architecture (" + std::to_string(machine) + 
                        ") is not AArch64 (EM_AARCH64 = 183). Cannot package for Nintendo Switch.";
        return false;
    }

    return true;
}

bool NroPackBackend::is_valid_jpeg(const std::string& image_path, std::string& error_message) {
    if (!fs::exists(image_path)) {
        error_message = "Icon file does not exist at: " + image_path;
        return false;
    }
    if (!fs::is_regular_file(image_path)) {
        error_message = "Icon path is not a regular file: " + image_path;
        return false;
    }

    std::ifstream file(image_path, std::ios::binary);
    if (!file.is_open()) {
        error_message = "Unable to open icon file for reading: " + image_path;
        return false;
    }

    unsigned char magic[4] = {0};
    file.read(reinterpret_cast<char*>(magic), 4);
    std::streamsize read_bytes = file.gcount();

    if (read_bytes < 3) {
        error_message = "Icon file is too small to be a valid image";
        return false;
    }

    // JPEG SOI magic is 0xFF, 0xD8, 0xFF
    if (magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF) {
        return true;
    }

    // Friendly diagnostics for common non-JPEG formats
    if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        error_message = "Application icon at '" + image_path + "' is in PNG format. Nintendo Switch NRO icons must be in JPEG format (.jpg/.jpeg).";
        return false;
    }
    if (magic[0] == 'B' && magic[1] == 'M') {
        error_message = "Application icon at '" + image_path + "' is in BMP format. Nintendo Switch NRO icons must be in JPEG format (.jpg/.jpeg).";
        return false;
    }

    error_message = "Application icon at '" + image_path + "' is not a valid JPEG image (missing 0xFFD8FF SOI magic).";
    return false;
}

bool NroPackBackend::validate_nro_binary(const std::string& nro_path, std::string& error_message) {
    if (!fs::exists(nro_path)) {
        error_message = "NRO output file does not exist at: " + nro_path;
        return false;
    }
    if (!fs::is_regular_file(nro_path)) {
        error_message = "NRO output path is not a regular file: " + nro_path;
        return false;
    }

    auto size = fs::file_size(nro_path);
    if (size < 0x20) {
        error_message = "Generated NRO file is too small (" + std::to_string(size) + " bytes)";
        return false;
    }

    std::ifstream file(nro_path, std::ios::binary);
    if (!file.is_open()) {
        error_message = "Unable to open generated NRO file for validation: " + nro_path;
        return false;
    }

    file.seekg(0x10);
    char magic[4] = {0};
    file.read(magic, 4);

    if (magic[0] != 'N' || magic[1] != 'R' || magic[2] != 'O' || magic[3] != '0') {
        error_message = "Generated NRO file does not contain valid 'NRO0' header magic at offset 0x10";
        return false;
    }

    return true;
}

PackageResult NroPackBackend::pack(
    const PackageRequest& request,
    [[maybe_unused]] const env::Environment* env,
    ProgressCallback progress
) {
    PackageResult res;
    res.format = PackageFormat::NRO;
    res.profile = request.profile;
    res.input_elf_path = request.input_elf_path;

    auto notify = [&](PackageStage stage, std::string_view msg) {
        res.log_messages.push_back("[" + package_stage_to_string(stage) + "] " + std::string(msg));
        if (progress) {
            progress(stage, msg);
        }
    };

    // -------------------------------------------------------------------------
    // Stage 1: Preflight & Tool Resolution
    // -------------------------------------------------------------------------
    notify(PackageStage::Preflight, "Validating input ELF binary and resolving switch-tools");

    // Discover tools
    std::string nacptool_path;
    std::string elf2nro_path;

    if (!request.nacptool_path_override.empty()) {
        nacptool_path = request.nacptool_path_override;
    } else {
        std::vector<std::string> candidates;
        if (const char* dkp = std::getenv("DEVKITPRO")) {
            candidates.push_back(std::string(dkp) + "/tools/bin/nacptool");
        }
        candidates.push_back("/opt/devkitpro/tools/bin/nacptool");
        candidates.push_back("/usr/bin/nacptool");
        candidates.push_back("/usr/local/bin/nacptool");
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                nacptool_path = c;
                break;
            }
        }
    }

    if (!request.elf2nro_path_override.empty()) {
        elf2nro_path = request.elf2nro_path_override;
    } else {
        std::vector<std::string> candidates;
        if (const char* dkp = std::getenv("DEVKITPRO")) {
            candidates.push_back(std::string(dkp) + "/tools/bin/elf2nro");
        }
        candidates.push_back("/opt/devkitpro/tools/bin/elf2nro");
        candidates.push_back("/usr/bin/elf2nro");
        candidates.push_back("/usr/local/bin/elf2nro");
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                elf2nro_path = c;
                break;
            }
        }
    }

    if (!request.dry_run) {
        if (nacptool_path.empty() || !fs::exists(nacptool_path)) {
            res.success = false;
            res.error_code = PackErrorCode::ToolNotFound;
            res.error_message = "devkitPro 'nacptool' not found. Please ensure switch-tools is installed ('dkp-pacman -S switch-tools').";
            return res;
        }
        if (elf2nro_path.empty() || !fs::exists(elf2nro_path)) {
            res.success = false;
            res.error_code = PackErrorCode::ToolNotFound;
            res.error_message = "devkitPro 'elf2nro' not found. Please ensure switch-tools is installed ('dkp-pacman -S switch-tools').";
            return res;
        }
    }

    // Validate input ELF binary if not dry-run
    if (!request.dry_run) {
        std::string elf_err;
        if (!validate_elf_binary(request.input_elf_path, elf_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidInputBinary;
            res.error_message = elf_err;
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 2: Staging & Output Paths
    // -------------------------------------------------------------------------
    std::string project_root = request.project_root.empty() ? fs::current_path().string() : request.project_root;
    std::string staging_dir = (fs::path(project_root) / ".nxdev" / "package" / "nro" / request.profile).string();
    std::string nacp_path = (fs::path(staging_dir) / "app.nacp").string();
    std::string staging_nro_path = (fs::path(staging_dir) / "staging.nro").string();

    std::string app_name = request.manifest.application().name.empty() ? 
                           fs::path(project_root).filename().string() : 
                           request.manifest.application().name;
    std::string safe_app_name = sanitize_filename(app_name);

    std::string final_output_path;
    if (!request.output_path.empty()) {
        final_output_path = request.output_path;
    } else {
        final_output_path = (fs::path(project_root) / "dist" / request.profile / (safe_app_name + ".nro")).string();
    }
    res.output_file = final_output_path;

    if (!request.dry_run) {
        std::error_code ec;
        fs::create_directories(staging_dir, ec);
        fs::create_directories(fs::path(final_output_path).parent_path(), ec);
    }

    // -------------------------------------------------------------------------
    // Stage 3: Generate NACP Metadata
    // -------------------------------------------------------------------------
    notify(PackageStage::GenerateNACP, "Generating NACP control metadata via nacptool");

    std::string author = request.manifest.application().author.empty() ? "Unspecified Author" : request.manifest.application().author;
    std::string version = request.manifest.application().version.empty() ? "1.0.0" : request.manifest.application().version;
    std::string title_id = request.manifest.application().format_title_id();

    std::vector<std::string> nacp_args = {
        "--create",
        app_name,
        author,
        version,
        nacp_path
    };

    if (!title_id.empty()) {
        nacp_args.push_back("--titleid=" + title_id);
    }

    res.nacp_file = nacp_path;

    if (!request.dry_run) {
        auto nacp_proc = ProcessExecutor::execute(nacptool_path, nacp_args, 30000);
        if (!nacp_proc.success || nacp_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NacpGenerationFailed;
            res.exit_code = nacp_proc.exit_code;
            res.error_message = "Failed to generate NACP metadata: " + 
                                (nacp_proc.stderr_output.empty() ? nacp_proc.stdout_output : nacp_proc.stderr_output);
            return res;
        }

        if (!fs::exists(nacp_path) || fs::file_size(nacp_path) == 0) {
            res.success = false;
            res.error_code = PackErrorCode::NacpGenerationFailed;
            res.error_message = "nacptool succeeded but output NACP file is missing or empty";
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 4: Asset Resolution (Icon & RomFS)
    // -------------------------------------------------------------------------
    notify(PackageStage::ResolveAssets, "Resolving and validating icon and RomFS assets");

    std::string resolved_icon_path;
    const auto& icon_cfg = request.manifest.assets().icon;
    if (icon_cfg.type == manifest::IconSourceType::ProjectFile && !icon_cfg.raw_path.empty()) {
        std::string p_icon = !icon_cfg.resolved_path.empty() ?
                             icon_cfg.resolved_path :
                             (fs::path(project_root) / icon_cfg.raw_path).string();
        std::string icon_err;
        if (!is_valid_jpeg(p_icon, icon_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidIcon;
            res.error_message = icon_err;
            return res;
        }
        resolved_icon_path = p_icon;
        res.icon_source = "project";
    } else {
        // Fallback to devkitPro / libnx default icon
        std::vector<std::string> icon_candidates;
        if (!request.default_icon_path_override.empty()) {
            icon_candidates.push_back(request.default_icon_path_override);
        }
        if (const char* dkp = std::getenv("DEVKITPRO")) {
            icon_candidates.push_back(std::string(dkp) + "/libnx/default_icon.jpg");
        }
        icon_candidates.push_back("/opt/devkitpro/libnx/default_icon.jpg");

        for (const auto& ic : icon_candidates) {
            if (fs::exists(ic)) {
                resolved_icon_path = ic;
                res.icon_source = "libnx_default";
                break;
            }
        }
        if (resolved_icon_path.empty()) {
            res.icon_source = "none";
        }
    }
    res.icon_file = resolved_icon_path;

    // RomFS Resolution via Shared Staging Pipeline
    std::optional<std::string> user_romfs_opt;
    const auto& romfs_cfg = request.manifest.assets().romfs;
    if (romfs_cfg.enabled && !romfs_cfg.raw_path.empty()) {
        std::string p_romfs = !romfs_cfg.resolved_path.empty() ?
                              romfs_cfg.resolved_path :
                              (fs::path(project_root) / romfs_cfg.raw_path).string();
        user_romfs_opt = p_romfs;
    }

    std::string canonical_staged_romfs;
    if (!request.dry_run) {
        RomFsStageRequest stage_req;
        stage_req.project_root = project_root;
        stage_req.output_dir = (fs::path(project_root) / ".nxdev" / "build" / request.profile / "romfs").string();
        stage_req.user_romfs_path = user_romfs_opt;
        stage_req.framework_layers = RomFsStager::resolve_manifest_layers(request.manifest, project_root);
        stage_req.clean = true;
        stage_req.verbose = request.verbose;

        auto stage_res = RomFsStager::stage(stage_req);
        if (!stage_res.success) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidRomFS;
            res.error_message = stage_res.error_message;
            return res;
        }

        if (stage_res.has_romfs) {
            canonical_staged_romfs = stage_res.staged_dir;
            res.romfs_dir = canonical_staged_romfs;
            res.romfs_manifest_file = stage_res.manifest_path;
            res.romfs_fingerprint = stage_res.fingerprint;
            res.romfs_files_count = stage_res.files_copied;
            res.romfs_overrides_count = stage_res.overridden_files;
            res.romfs_layers = stage_res.source_layers;
        }
    } else {
        if (user_romfs_opt.has_value() || !RomFsStager::resolve_manifest_layers(request.manifest, project_root).empty()) {
            canonical_staged_romfs = (fs::path(project_root) / ".nxdev" / "build" / request.profile / "romfs").string();
            res.romfs_dir = canonical_staged_romfs;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 5: Create NRO Binary (elf2nro)
    // -------------------------------------------------------------------------
    notify(PackageStage::CreateNRO, "Converting ELF to NRO binary via elf2nro");

    std::vector<std::string> elf2nro_args = {
        request.input_elf_path,
        staging_nro_path,
        "--nacp=" + nacp_path
    };

    if (!resolved_icon_path.empty()) {
        elf2nro_args.push_back("--icon=" + resolved_icon_path);
    }
    if (!canonical_staged_romfs.empty()) {
        elf2nro_args.push_back("--romfsdir=" + canonical_staged_romfs);
    }

    if (!request.dry_run) {
        auto elf2nro_proc = ProcessExecutor::execute(elf2nro_path, elf2nro_args, 120000);
        if (!elf2nro_proc.success || elf2nro_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.exit_code = elf2nro_proc.exit_code;
            res.error_message = "elf2nro packaging failed: " + 
                                (elf2nro_proc.stderr_output.empty() ? elf2nro_proc.stdout_output : elf2nro_proc.stderr_output);
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 6: Validation & Atomic Finalize
    // -------------------------------------------------------------------------
    notify(PackageStage::Validate, "Validating NRO header and structure");

    if (!request.dry_run) {
        std::string nro_err;
        if (!validate_nro_binary(staging_nro_path, nro_err)) {
            res.success = false;
            res.error_code = PackErrorCode::ValidationFailed;
            res.error_message = nro_err;
            return res;
        }

        notify(PackageStage::Finalize, "Placing final NRO artifact at " + final_output_path);

        std::error_code ec;
        if (fs::exists(final_output_path)) {
            fs::remove(final_output_path, ec);
        }
        fs::rename(staging_nro_path, final_output_path, ec);
        if (ec) {
            // Fallback to copy & remove across filesystems
            fs::copy_file(staging_nro_path, final_output_path, fs::copy_options::overwrite_existing, ec);
            fs::remove(staging_nro_path, ec);
        }

        res.file_size_bytes = fs::file_size(final_output_path);
    } else {
        res.file_size_bytes = 0;
    }

    res.success = true;
    res.error_code = PackErrorCode::None;
    return res;
}

} // namespace nxdev::pack
