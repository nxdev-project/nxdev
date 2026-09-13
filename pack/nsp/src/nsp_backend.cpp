#include <nxdev/pack/nsp_backend.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/process.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace fs = std::filesystem;

namespace nxdev::pack {

std::string HacBrewPackAdapter::sanitize_filename(std::string_view name) {
    return NroPackBackend::sanitize_filename(name);
}

bool HacBrewPackAdapter::validate_pfs0_binary(const std::string& nsp_path, std::string& error_msg) {
    if (!fs::exists(nsp_path)) {
        error_msg = "NSP file does not exist at path: " + nsp_path;
        return false;
    }
    std::error_code ec;
    auto size = fs::file_size(nsp_path, ec);
    if (ec || size < 16) {
        error_msg = "NSP file is too small or inaccessible (size: " + std::to_string(size) + " bytes)";
        return false;
    }

    std::ifstream in(nsp_path, std::ios::binary);
    if (!in.is_open()) {
        error_msg = "Unable to open NSP binary for reading: " + nsp_path;
        return false;
    }

    char magic[4] = {0};
    in.read(magic, 4);
    if (magic[0] != 'P' || magic[1] != 'F' || magic[2] != 'S' || magic[3] != '0') {
        error_msg = "Invalid NSP binary: missing 'PFS0' container magic header";
        return false;
    }

    uint32_t num_files = 0;
    in.read(reinterpret_cast<char*>(&num_files), sizeof(uint32_t));
    if (num_files == 0) {
        error_msg = "Invalid NSP binary: PFS0 container has zero files";
        return false;
    }

    return true;
}

std::string HacBrewPackAdapter::resolve_keys_file(
    const PackageRequest& request,
    [[maybe_unused]] const env::Environment* env
) {
    // 1. Explicit request override
    if (!request.keys_path.empty()) {
        if (fs::exists(request.keys_path)) {
            return request.keys_path;
        }
        return "";
    }

    // 2. NXDEV_KEYS environment variable
    if (const char* env_keys = std::getenv("NXDEV_KEYS")) {
        if (std::strlen(env_keys) > 0 && fs::exists(env_keys)) {
            return std::string(env_keys);
        }
    }

    // 3. Project-local keys (e.g. .nxdev/prod.keys or keys.dat)
    if (!request.project_root.empty()) {
        std::vector<std::string> proj_candidates = {
            (fs::path(request.project_root) / ".nxdev" / "prod.keys").string(),
            (fs::path(request.project_root) / ".nxdev" / "keys.dat").string(),
            (fs::path(request.project_root) / "prod.keys").string(),
            (fs::path(request.project_root) / "keys.dat").string()
        };
        for (const auto& c : proj_candidates) {
            if (fs::exists(c)) return c;
        }
    }

    // 4. Standard Switch default locations (~/.switch/prod.keys, ~/.switch/keys.dat)
    if (const char* home = std::getenv("HOME")) {
        std::vector<std::string> home_candidates = {
            (fs::path(home) / ".switch" / "prod.keys").string(),
            (fs::path(home) / ".switch" / "keys.dat").string(),
            (fs::path(home) / ".switch" / "keys.ini").string()
        };
        for (const auto& c : home_candidates) {
            if (fs::exists(c)) return c;
        }
    }

    return "";
}

bool HacBrewPackAdapter::generate_npdm_json(
    const manifest::Manifest& manifest,
    std::string& out_json,
    std::string& error_msg
) {
    const auto& app = manifest.application();
    const auto& npdm = manifest.npdm();

    std::string title_id = app.title_id.value_or("0100000000000001");
    std::string app_name = app.name.empty() ? "Application" : app.name;

    // Services list based on preset and explicit overrides
    std::vector<std::pair<std::string, bool>> services;
    auto add_service = [&](const std::string& srv, bool is_host = false) {
        if (srv.length() < 1 || srv.length() > 8) return;
        for (const auto& existing : services) {
            if (existing.first == srv) return;
        }
        services.push_back({srv, is_host});
    };

    // Baseline standard services
    add_service("sm:");
    add_service("bsds:u");
    add_service("acc:u");
    add_service("set");
    add_service("hid");
    add_service("fsp-srv");
    add_service("nvnflinger");
    add_service("vi:u");
    add_service("appletOE");
    add_service("audren:u");
    add_service("audin:u");
    add_service("audout:u");
    add_service("time:u");
    add_service("psm");
    add_service("pl:u");

    if (npdm.preset == manifest::NpdmPreset::Network || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("bsds:s");
        add_service("bsdcfg");
        add_service("sfdnsres");
        add_service("nifm:u");
        add_service("ssl:u");
    }

    if (npdm.preset == manifest::NpdmPreset::Filesystem || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("fsp-pr");
        add_service("fsp-ldr");
    }

    if (npdm.preset == manifest::NpdmPreset::Multimedia || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("hwopus");
        add_service("caps:u");
        add_service("caps:a");
    }

    // Add manifest explicit services
    for (const auto& s : npdm.services) {
        if (s.length() < 1 || s.length() > 8) {
            error_msg = "Invalid service name '" + s + "': service names must be between 1 and 8 characters.";
            return false;
        }
        add_service(s);
    }

    // Validate main thread settings
    int priority = npdm.main_thread.priority;
    if (priority < 0 || priority > 63) {
        error_msg = "Invalid main thread priority " + std::to_string(priority) + ": must be between 0 and 63.";
        return false;
    }

    int core = npdm.main_thread.core;
    if (core < 0 || core > 3) {
        error_msg = "Invalid main thread CPU core " + std::to_string(core) + ": must be between 0 and 3.";
        return false;
    }

    uint32_t stack_size = npdm.main_thread.stack_size;
    if (stack_size < 0x1000) {
        error_msg = "Invalid main thread stack size " + std::to_string(stack_size) + ": minimum 4096 bytes required.";
        return false;
    }

    // Format hex values
    std::string hex_tid = title_id;
    if (hex_tid.rfind("0x", 0) != 0 && hex_tid.rfind("0X", 0) != 0) {
        hex_tid = "0x" + hex_tid;
    }

    std::ostringstream stack_hex;
    stack_hex << "0x" << std::setfill('0') << std::setw(8) << std::hex << stack_size;

    // Build JSON
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << app_name << "\",\n";
    ss << "  \"title_id\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id_range_min\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id_range_max\": \"" << hex_tid << "\",\n";
    ss << "  \"main_thread_stack_size\": \"" << stack_hex.str() << "\",\n";
    ss << "  \"main_thread_priority\": " << priority << ",\n";
    ss << "  \"default_cpu_id\": " << core << ",\n";
    ss << "  \"process_category\": 0,\n";
    ss << "  \"is_64_bit\": true,\n";
    ss << "  \"address_space_type\": 1,\n";
    ss << "  \"system_resource_size\": \"0x00000000\",\n";
    ss << "  \"version\": \"0x00010000\",\n";
    ss << "  \"optimize_memory_allocation\": false,\n";
    ss << "  \"disable_device_address_space_merge\": false,\n";
    ss << "  \"enable_alias_region_extra_size\": false,\n";
    ss << "  \"prevent_code_reads\": false,\n";
    ss << "  \"signature_key_generation\": 0,\n";
    ss << "  \"is_retail\": false,\n";
    ss << "  \"pool_partition\": 0,\n";
    ss << "  \"filesystem_access\": {\n";
    ss << "    \"permissions\": \"0x0000000000000000\"\n";
    ss << "  },\n";
    ss << "  \"service_access\": [\n";
    ss << "    \"*\"\n";
    ss << "  ],\n";
    ss << "  \"service_host\": [],\n";
    ss << "  \"services\": {\n";
    for (size_t i = 0; i < services.size(); ++i) {
        ss << "    \"" << services[i].first << "\": " << (services[i].second ? "true" : "false");
        if (i + 1 < services.size()) ss << ",";
        ss << "\n";
    }
    ss << "  },\n";
    ss << "  \"kernel_capabilities\": [\n";
    ss << "    {\n";
    ss << "      \"type\": \"kernel_flags\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"highest_thread_priority\": 63,\n";
    ss << "        \"lowest_thread_priority\": 24,\n";
    ss << "        \"lowest_cpu_id\": 0,\n";
    ss << "        \"highest_cpu_id\": 3\n";
    ss << "      }\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"syscalls\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"svcSetHeapSize\": 1,\n";
    ss << "        \"svcSetMemoryAttribute\": 3,\n";
    ss << "        \"svcMapMemory\": 4,\n";
    ss << "        \"svcUnmapMemory\": 5,\n";
    ss << "        \"svcQueryMemory\": 6,\n";
    ss << "        \"svcExitProcess\": 7,\n";
    ss << "        \"svcCreateThread\": 8,\n";
    ss << "        \"svcStartThread\": 9,\n";
    ss << "        \"svcExitThread\": 10,\n";
    ss << "        \"svcSleepThread\": 11,\n";
    ss << "        \"svcGetThreadPriority\": 12,\n";
    ss << "        \"svcSetThreadPriority\": 13,\n";
    ss << "        \"svcGetThreadCoreMask\": 14,\n";
    ss << "        \"svcSetThreadCoreMask\": 15,\n";
    ss << "        \"svcGetCurrentProcessorNumber\": 16,\n";
    ss << "        \"svcSignalEvent\": 17,\n";
    ss << "        \"svcClearEvent\": 18,\n";
    ss << "        \"svcCreateInterruptEvent\": 30,\n";
    ss << "        \"svcMapSharedMemory\": 19,\n";
    ss << "        \"svcUnmapSharedMemory\": 20,\n";
    ss << "        \"svcCreateTransferMemory\": 21,\n";
    ss << "        \"svcCloseHandle\": 22,\n";
    ss << "        \"svcResetSignal\": 23,\n";
    ss << "        \"svcWaitSynchronization\": 24,\n";
    ss << "        \"svcCancelSynchronization\": 25,\n";
    ss << "        \"svcArbitrateLock\": 26,\n";
    ss << "        \"svcArbitrateUnlock\": 27,\n";
    ss << "        \"svcWaitProcessWideKeyAtomic\": 28,\n";
    ss << "        \"svcSignalProcessWideKey\": 29,\n";
    ss << "        \"svcGetSystemTick\": 31,\n";
    ss << "        \"svcConnectToNamedPort\": 32,\n";
    ss << "        \"svcSendSyncRequestLight\": 33,\n";
    ss << "        \"svcSendSyncRequest\": 34,\n";
    ss << "        \"svcSendSyncRequestWithUserBuffer\": 35,\n";
    ss << "        \"svcSendAsyncRequestWithUserBuffer\": 36,\n";
    ss << "        \"svcGetProcessId\": 37,\n";
    ss << "        \"svcGetThreadId\": 38,\n";
    ss << "        \"svcBreak\": 39,\n";
    ss << "        \"svcOutputDebugString\": 40,\n";
    ss << "        \"svcReturnFromException\": 41,\n";
    ss << "        \"svcGetInfo\": 43,\n";
    ss << "        \"svcWaitForAddress\": 52,\n";
    ss << "        \"svcSignalToAddress\": 53,\n";
    ss << "        \"svcSynchronizePreemptionState\": 90\n";
    ss << "      }\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"min_kernel_version\",\n";
    ss << "      \"value\": \"0x0030\"\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"handle_table_size\",\n";
    ss << "      \"value\": 1023\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"debug_flags\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"allow_debug\": true,\n";
    ss << "        \"force_debug\": false,\n";
    ss << "        \"force_debug_prod\": false\n";
    ss << "      }\n";
    ss << "    }\n";
    ss << "  ]\n";
    ss << "}\n";

    out_json = ss.str();
    return true;
}

PackageResult NspPackBackend::pack(
    const PackageRequest& request,
    const env::Environment* env,
    ProgressCallback progress
) {
    PackageResult res;
    res.format = PackageFormat::NSP;
    res.profile = request.profile;
    res.backend_name = HacBrewPackAdapter::DEFAULT_BACKEND_NAME;
    res.backend_version = HacBrewPackAdapter::PINNED_BACKEND_VERSION;

    auto notify = [&](PackageStage stage, std::string_view msg) {
        res.log_messages.push_back("[" + package_stage_to_string(stage) + "] " + std::string(msg));
        if (progress) {
            progress(stage, msg);
        }
    };

    // -------------------------------------------------------------------------
    // Stage 1: Validation & Prerequisites
    // -------------------------------------------------------------------------
    notify(PackageStage::Preflight, "Validating NSP identity, Title ID, and Switch keys");

    // 1.1 Title ID requirement
    const auto& app = request.manifest.application();
    if (!app.title_id.has_value() || app.title_id->empty()) {
        res.success = false;
        res.error_code = PackErrorCode::MissingTitleId;
        res.error_message = "NSP packaging requires application.titleId configured in nxapp.yaml (e.g. titleId: \"0100000000000088\").";
        return res;
    }
    std::string raw_tid = app.title_id.value();
    if (raw_tid.length() != 16) {
        res.success = false;
        res.error_code = PackErrorCode::InvalidTitleId;
        res.error_message = "Invalid Title ID '" + raw_tid + "': Title ID must be exactly 16 hexadecimal characters.";
        return res;
    }
    for (char c : raw_tid) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidTitleId;
            res.error_message = "Invalid Title ID '" + raw_tid + "': contains non-hexadecimal character '" + std::string(1, c) + "'.";
            return res;
        }
    }
    res.title_id = raw_tid;

    // 1.2 Switch keys requirement
    std::string keys_file = HacBrewPackAdapter::resolve_keys_file(request, env);
    if (keys_file.empty()) {
        if (request.dry_run) {
            notify(PackageStage::Preflight, "Note: No Switch key file found. Real packaging will require prod.keys.");
            keys_file = "<prod.keys>";
        } else {
            res.success = false;
            res.error_code = PackErrorCode::KeyFileMissing;
            res.error_message = "NSP packaging requires a user-provided Switch key file (prod.keys).\n"
                                "Please configure a key file via:\n"
                                "  - CLI option: nxdev pack nsp --keys <path/to/prod.keys>\n"
                                "  - Environment variable: export NXDEV_KEYS=<path/to/prod.keys>\n"
                                "  - Placing prod.keys in ~/.switch/prod.keys";
            return res;
        }
    }

    // 1.3 Validate input ELF binary
    if (!request.dry_run) {
        std::string elf_err;
        if (!NroPackBackend::validate_elf_binary(request.input_elf_path, elf_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidInputBinary;
            res.error_message = elf_err;
            return res;
        }
    }
    res.input_elf_path = request.input_elf_path;

    // -------------------------------------------------------------------------
    // Stage 2: Tool Discovery
    // -------------------------------------------------------------------------
    std::string elf2nso_bin;
    std::string npdmtool_bin;
    std::string nacptool_bin;
    std::string hacbrewpack_bin;

    auto find_tool = [&](const std::string& override_path, const std::string& name) -> std::string {
        if (!override_path.empty() && fs::exists(override_path)) return override_path;
        try {
            if (fs::exists("/proc/self/exe")) {
                auto self_dir = fs::canonical("/proc/self/exe").parent_path();
                if (fs::exists(self_dir / name)) {
                    return (self_dir / name).string();
                }
            }
        } catch (...) {}
        if (const char* dkp = std::getenv("DEVKITPRO")) {
            std::string p = (fs::path(dkp) / "tools" / "bin" / name).string();
            if (fs::exists(p)) return p;
        }
        std::string std_p = (fs::path("/opt/devkitpro/tools/bin") / name).string();
        if (fs::exists(std_p)) return std_p;

        // Check project build directory for bundled host tools (e.g. hacbrewpack)
        if (!request.project_root.empty()) {
            std::vector<std::string> local_paths = {
                (fs::path(request.project_root) / "build" / "bin" / name).string(),
                (fs::path(request.project_root) / ".nxdev" / "bin" / name).string(),
                (fs::path(request.project_root) / ".." / "build" / "bin" / name).string(),
                (fs::path(request.project_root) / ".." / ".." / "build" / "bin" / name).string()
            };
            for (const auto& lp : local_paths) {
                if (fs::exists(lp)) return lp;
            }
        }
        return name;
    };

    elf2nso_bin = find_tool(request.elf2nso_path_override, "elf2nso");
    npdmtool_bin = find_tool(request.npdmtool_path_override, "npdmtool");
    nacptool_bin = find_tool(request.nacptool_path_override, "nacptool");
    hacbrewpack_bin = find_tool(request.hacbrewpack_path_override, "hacbrewpack");

    // -------------------------------------------------------------------------
    // Stage 3: Staging Workspace Creation
    // -------------------------------------------------------------------------
    std::string project_root = request.project_root.empty() ? "." : request.project_root;
    fs::path nsp_pkg_root = fs::path(project_root) / ".nxdev" / "package" / "nsp" / request.profile;
    fs::path staging_dir = nsp_pkg_root / "staging";
    fs::path exefs_dir = staging_dir / "exefs";
    fs::path control_dir = staging_dir / "control";
    fs::path romfs_staging_dir = staging_dir / "romfs";
    fs::path gen_dir = nsp_pkg_root / "generated";
    fs::path output_staging_dir = nsp_pkg_root / "output";

    if (!request.dry_run) {
        fs::create_directories(exefs_dir);
        fs::create_directories(control_dir);
        fs::create_directories(gen_dir);
        fs::create_directories(output_staging_dir);
    }

    // -------------------------------------------------------------------------
    // Stage 4: Prepare ExeFS (NSO binary & NPDM)
    // -------------------------------------------------------------------------
    notify(PackageStage::PrepareExeFS, "Converting ELF binary to NSO and staging ExeFS");

    std::string nso_path = (exefs_dir / "main").string();
    res.nso_file = nso_path;

    if (!request.dry_run) {
        // Run elf2nso <elf_file> <nso_file>
        auto nso_res = ProcessExecutor::execute(elf2nso_bin, {request.input_elf_path, nso_path}, 30000);
        if (!nso_res.success || nso_res.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NsoConversionFailed;
            res.exit_code = nso_res.exit_code;
            res.error_message = "Failed to convert ELF to NSO via elf2nso: " +
                                (nso_res.stderr_output.empty() ? nso_res.stdout_output : nso_res.stderr_output);
            return res;
        }
    }

    notify(PackageStage::GenerateNPDM, "Generating NPDM metadata specification");
    std::string npdm_json_content;
    std::string npdm_err;
    if (!HacBrewPackAdapter::generate_npdm_json(request.manifest, npdm_json_content, npdm_err)) {
        res.success = false;
        res.error_code = PackErrorCode::InvalidNpdm;
        res.error_message = npdm_err;
        return res;
    }

    std::string npdm_json_file = (gen_dir / "npdm.json").string();
    std::string npdm_bin_file = (exefs_dir / "main.npdm").string();
    res.npdm_file = npdm_bin_file;

    if (!request.dry_run) {
        {
            std::ofstream jf(npdm_json_file);
            jf << npdm_json_content;
        }

        // Run npdmtool <json> <out_npdm>
        auto npdm_res = ProcessExecutor::execute(npdmtool_bin, {npdm_json_file, npdm_bin_file}, 30000);
        if (!npdm_res.success || npdm_res.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NpdmGenerationFailed;
            res.exit_code = npdm_res.exit_code;
            res.error_message = "Failed to generate NPDM binary via npdmtool: " +
                                (npdm_res.stderr_output.empty() ? npdm_res.stdout_output : npdm_res.stderr_output);
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 5: Generate Control Metadata (NACP & Icon)
    // -------------------------------------------------------------------------
    notify(PackageStage::GenerateNACP, "Generating NACP control metadata");

    std::string nacp_path = (control_dir / "control.nacp").string();
    res.nacp_file = nacp_path;

    if (!request.dry_run) {
        std::vector<std::string> nacp_args = {
            "--create",
            app.name.empty() ? "Application" : app.name,
            app.author.empty() ? "Unspecified Author" : app.author,
            app.version.empty() ? "1.0.0" : app.version,
            nacp_path,
            "--titleid=" + raw_tid
        };
        auto nacp_proc = ProcessExecutor::execute(nacptool_bin, nacp_args, 30000);
        if (!nacp_proc.success || nacp_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NacpGenerationFailed;
            res.exit_code = nacp_proc.exit_code;
            res.error_message = "Failed to generate NACP metadata: " +
                                (nacp_proc.stderr_output.empty() ? nacp_proc.stdout_output : nacp_proc.stderr_output);
            return res;
        }
    }

    notify(PackageStage::ResolveAssets, "Resolving and validating icon and RomFS assets");

    // Icon handling
    std::string resolved_icon_path;
    const auto& icon_cfg = request.manifest.assets().icon;
    if (icon_cfg.type == manifest::IconSourceType::ProjectFile && !icon_cfg.raw_path.empty()) {
        std::string p_icon = !icon_cfg.resolved_path.empty() ?
                             icon_cfg.resolved_path :
                             (fs::path(project_root) / icon_cfg.raw_path).string();
        std::string icon_err;
        if (!request.dry_run && !NroPackBackend::is_valid_jpeg(p_icon, icon_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidIcon;
            res.error_message = icon_err;
            return res;
        }
        resolved_icon_path = p_icon;
        res.icon_source = "project";
    } else {
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
    }

    if (!resolved_icon_path.empty()) {
        res.icon_file = resolved_icon_path;
        if (!request.dry_run) {
            std::string dest_icon = (control_dir / "icon_AmericanEnglish.dat").string();
            std::error_code ec;
            fs::copy_file(resolved_icon_path, dest_icon, fs::copy_options::overwrite_existing, ec);
        }
    }

    // RomFS handling
    bool has_romfs = false;
    const auto& romfs_cfg = request.manifest.assets().romfs;
    if (romfs_cfg.enabled || !romfs_cfg.raw_path.empty()) {
        std::string p_romfs = !romfs_cfg.resolved_path.empty() ?
                              romfs_cfg.resolved_path :
                              (fs::path(project_root) / romfs_cfg.raw_path).string();
        if (!fs::exists(p_romfs) || !fs::is_directory(p_romfs)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidRomFS;
            res.error_message = "Configured RomFS directory does not exist or is not a directory: " + p_romfs;
            return res;
        }
        has_romfs = true;
        res.romfs_dir = p_romfs;
        if (!request.dry_run) {
            std::error_code ec;
            fs::create_directories(romfs_staging_dir);
            fs::copy(p_romfs, romfs_staging_dir, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        }
    }

    // -------------------------------------------------------------------------
    // Stage 6: Build NSP via hacBrewPack
    // -------------------------------------------------------------------------
    notify(PackageStage::CreateNSP, "Invoking hacBrewPack backend to construct NSP package");

    std::string safe_name = HacBrewPackAdapter::sanitize_filename(app.name.empty() ? fs::path(project_root).filename().string() : app.name);
    std::string final_destination;
    if (!request.output_path.empty()) {
        final_destination = request.output_path;
    } else {
        fs::path dist_dir = fs::path(project_root) / "dist" / request.profile;
        final_destination = (dist_dir / (safe_name + ".nsp")).string();
    }

    if (!request.dry_run) {
        std::vector<std::string> pack_args = {
            "--titleid=" + raw_tid,
            "--keyfile=" + keys_file,
            "--exefsdir=" + exefs_dir.string(),
            "--controldir=" + control_dir.string(),
            "--outdir=" + output_staging_dir.string()
        };

        if (has_romfs) {
            pack_args.push_back("--romfsdir=" + romfs_staging_dir.string());
        } else {
            pack_args.push_back("--noromfs");
        }

        auto pack_proc = ProcessExecutor::execute(hacbrewpack_bin, pack_args, 60000);
        if (!pack_proc.success || pack_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.exit_code = pack_proc.exit_code;
            res.error_message = "hacBrewPack execution failed: " +
                                (pack_proc.stderr_output.empty() ? pack_proc.stdout_output : pack_proc.stderr_output);
            return res;
        }

        // -------------------------------------------------------------------------
        // Stage 7: Validation & Final Placement
        // -------------------------------------------------------------------------
        notify(PackageStage::Validate, "Validating NSP PFS0 container structure");

        std::string raw_tid_lower = raw_tid;
        std::transform(raw_tid_lower.begin(), raw_tid_lower.end(), raw_tid_lower.begin(), [](unsigned char c) { return std::tolower(c); });
        
        std::vector<std::string> staged_nsp_candidates = {
            (output_staging_dir / (raw_tid_lower + ".nsp")).string(),
            (output_staging_dir / (raw_tid + ".nsp")).string(),
            (output_staging_dir / (safe_name + ".nsp")).string()
        };

        std::string found_staged_nsp;
        for (const auto& c : staged_nsp_candidates) {
            if (fs::exists(c)) {
                found_staged_nsp = c;
                break;
            }
        }

        if (found_staged_nsp.empty()) {
            // Check for any .nsp file in output staging directory
            if (fs::exists(output_staging_dir) && fs::is_directory(output_staging_dir)) {
                for (const auto& entry : fs::directory_iterator(output_staging_dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".nsp") {
                        found_staged_nsp = entry.path().string();
                        break;
                    }
                }
            }
        }

        if (found_staged_nsp.empty()) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.error_message = "hacBrewPack completed but no output .nsp file was found in output staging directory.";
            return res;
        }

        std::string pfs0_err;
        if (!HacBrewPackAdapter::validate_pfs0_binary(found_staged_nsp, pfs0_err)) {
            res.success = false;
            res.error_code = PackErrorCode::ValidationFailed;
            res.error_message = pfs0_err;
            return res;
        }

        notify(PackageStage::Finalize, "Placing final NSP artifact at " + final_destination);
        fs::path dest_parent = fs::path(final_destination).parent_path();
        if (!dest_parent.empty()) {
            fs::create_directories(dest_parent);
        }

        std::error_code ec;
        fs::copy_file(found_staged_nsp, final_destination, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.error_message = "Failed to copy final NSP artifact to destination: " + ec.message();
            return res;
        }

        res.file_size_bytes = fs::file_size(final_destination, ec);
    } else {
        notify(PackageStage::Finalize, "[Dry-Run] Simulated placement at " + final_destination);
    }

    res.success = true;
    res.output_file = final_destination;
    return res;
}

} // namespace nxdev::pack
