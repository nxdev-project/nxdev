#include <nxdev/cli/commands.hpp>
#include <nxdev/pack/pack.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/build/build_orchestrator.hpp>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace nxdev::cli {

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

int PackCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    bool json_output = false;
    bool dry_run = false;
    bool no_build = false;
    bool force = false;
    bool verbose = ctx.verbose;
    bool keep_staging = false;
    bool debug_backend = false;
    std::string profile_str;
    std::string output_path;
    std::string keys_path;
    std::string requested_format;
    std::vector<std::string> positional;

    for (size_t i = 0; i < args.size(); ++i) {
        const auto& a = args[i];
        if (a == "-h" || a == "--help" || a == "help") {
            std::cout << "NXDevPack - Package Switch Homebrew Binaries into NRO or NSP\n\n"
                      << "Usage: nxdev pack [format] [options]\n\n"
                      << "Formats:\n"
                      << "  nro (default)                  Package into Homebrew Menu NRO executable\n"
                      << "  nsp                            Package into installable NSP (via hacBrewPack)\n\n"
                      << "Options:\n"
                      << "  --keys <path>                  Path to user-provided Switch keys file (prod.keys)\n"
                      << "  --profile <debug|release>      Build profile to package (default: debug)\n"
                      << "  -o, --output <path>            Explicit target destination path for final artifact\n"
                      << "  --no-build                     Skip compilation and package existing ELF binary\n"
                      << "  -f, --force                    Force overwrite / repackage\n"
                      << "  --keep-staging                 Retain backend staging directories after packaging\n"
                      << "  --debug-backend                Display detailed backend execution diagnostic info\n"
                      << "  --dry-run                      Simulate packaging steps without invoking tools\n"
                      << "  --json                         Output result in structured JSON format\n"
                      << "  -V, --verbose                  Enable detailed diagnostic stage logging\n"
                      << "  -h, --help                     Display this help menu\n";
            return 0;
        } else if (a == "--json") {
            json_output = true;
        } else if (a == "--dry-run") {
            dry_run = true;
        } else if (a == "--no-build") {
            no_build = true;
        } else if (a == "-f" || a == "--force") {
            force = true;
        } else if (a == "-V" || a == "--verbose") {
            verbose = true;
        } else if (a == "--keep-staging") {
            keep_staging = true;
        } else if (a == "--debug-backend") {
            debug_backend = true;
        } else if (a == "--profile") {
            if (i + 1 < args.size()) {
                profile_str = args[++i];
            } else {
                std::cerr << "Error: '--profile' requires a profile name (debug or release).\n";
                return 1;
            }
        } else if (a == "--keys" || a == "-k") {
            if (i + 1 < args.size()) {
                keys_path = args[++i];
            } else {
                std::cerr << "Error: '--keys' requires a file path argument to prod.keys.\n";
                return 1;
            }
        } else if (a == "-o" || a == "--output") {
            if (i + 1 < args.size()) {
                output_path = args[++i];
            } else {
                std::cerr << "Error: '--output' requires a file path argument.\n";
                return 1;
            }
        } else {
            positional.push_back(a);
        }
    }

    if (!positional.empty()) {
        std::string sub = positional[0];
        if (sub == "nro" || sub == "NRO") {
            requested_format = "nro";
        } else if (sub == "nsp" || sub == "NSP") {
            requested_format = "nsp";
        } else {
            std::cerr << "Error: Unknown packaging format '" << sub << "'. Valid formats: 'nro', 'nsp'.\n";
            return 1;
        }
    }

    // Check project presence
    if (!ctx.project.has_value() || !ctx.project->is_valid()) {
        if (json_output) {
            std::cout << "{\n  \"status\": \"error\",\n  \"error\": {\n    \"code\": \"NoProject\",\n    \"message\": \"No valid NXDev project found in current directory\"\n  }\n}\n";
        } else {
            std::cerr << "Error: 'nxdev pack' requires an active project containing a valid nxapp.yaml manifest.\n";
        }
        return 1;
    }

    const auto& project = ctx.project.value();
    const auto& manifest = project.manifest();

    // Default format from manifest if not given explicitly
    if (requested_format.empty()) {
        requested_format = (manifest.packaging().default_format == manifest::PackageFormat::NRO) ? "nro" : "nsp";
    }

    bool is_nsp = (requested_format == "nsp");

    // Resolve profile
    if (profile_str.empty()) {
        profile_str = manifest.build().default_profile.empty() ? "debug" : manifest.build().default_profile;
    }
    auto profile_opt = build::parse_build_profile(profile_str);
    if (!profile_opt.has_value()) {
        if (json_output) {
            std::cout << "{\n  \"status\": \"error\",\n  \"error\": {\n    \"code\": \"InvalidProfile\",\n    \"message\": \"Invalid build profile: " << escape_json_str(profile_str) << "\"\n  }\n}\n";
        } else {
            std::cerr << "Error: Invalid build profile '" << profile_str << "'. Valid profiles: 'debug', 'release'.\n";
        }
        return 1;
    }
    build::BuildProfile profile = profile_opt.value();
    std::string profile_name = build::build_profile_to_string(profile);

    // -------------------------------------------------------------------------
    // Step 1: Build ELF binary if needed
    // -------------------------------------------------------------------------
    std::string elf_path;

    if (!no_build) {
        if (!json_output) {
            std::cout << "\nNXDevPack - Application Packaging\n\n"
                      << "  Application: " << manifest.application().name << "\n"
                      << "  Format:      " << (is_nsp ? "NSP" : "NRO") << "\n"
                      << "  Profile:     " << profile_name << "\n\n"
                      << "[1/" << (is_nsp ? "7" : "4") << "] Build: Compiling Nintendo Switch AArch64 ELF binary...\n";
        }

        build::BuildOptions build_opts;
        build_opts.profile = profile;
        build_opts.verbose = verbose;

        auto build_res = build::BuildOrchestrator::build(project, ctx.env, build_opts);
        if (!build_res.success) {
            if (json_output) {
                std::cout << "{\n  \"status\": \"error\",\n  \"format\": \"" << requested_format << "\",\n  \"profile\": \"" << profile_name << "\",\n  \"error\": {\n    \"code\": \"BuildFailed\",\n    \"message\": \"Compilation of Switch ELF failed with exit code " << build_res.exit_code << "\",\n    \"exitCode\": " << build_res.exit_code << "\n  }\n}\n";
            } else {
                std::cerr << "\nError: Build failed with exit code " << build_res.exit_code << ".\n";
                if (!build_res.stderr_output.empty()) {
                    std::cerr << build_res.stderr_output << "\n";
                }
            }
            return build_res.exit_code != 0 ? build_res.exit_code : 1;
        }

        elf_path = build_res.elf_path;
    } else {
        // Resolve pre-existing ELF binary
        std::string build_dir = build::BuildOrchestrator::resolve_build_directory(project.root_path(), profile_name);
        std::string root_name = fs::path(project.root_path()).filename().string();
        std::string safe_name = pack::NroPackBackend::sanitize_filename(manifest.application().name);
        
        // Search candidates
        std::vector<std::string> elf_candidates = {
            (fs::path(build_dir) / "bin" / (root_name + ".elf")).string(),
            (fs::path(build_dir) / "bin" / (safe_name + ".elf")).string(),
            (fs::path(build_dir) / (root_name + ".elf")).string(),
            (fs::path(build_dir) / (safe_name + ".elf")).string()
        };

        for (const auto& c : elf_candidates) {
            if (fs::exists(c)) {
                elf_path = c;
                break;
            }
        }

        if (elf_path.empty()) {
            std::string bin_dir = (fs::path(build_dir) / "bin").string();
            if (fs::exists(bin_dir) && fs::is_directory(bin_dir)) {
                for (const auto& entry : fs::directory_iterator(bin_dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".elf") {
                        elf_path = entry.path().string();
                        break;
                    }
                }
            }
        }

        if (elf_path.empty() && fs::exists(build_dir) && fs::is_directory(build_dir)) {
            for (const auto& entry : fs::directory_iterator(build_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".elf") {
                    elf_path = entry.path().string();
                    break;
                }
            }
        }

        if (elf_path.empty()) {
            if (json_output) {
                std::cout << "{\n  \"status\": \"error\",\n  \"format\": \"" << requested_format << "\",\n  \"profile\": \"" << profile_name << "\",\n  \"error\": {\n    \"code\": \"MissingElf\",\n    \"message\": \"No existing ELF binary found in build directory. Run 'nxdev build' first or omit '--no-build'.\"\n  }\n}\n";
            } else {
                std::cerr << "Error: No existing ELF binary found in build directory ('" << build_dir << "').\n"
                          << "Please run 'nxdev build' first or run 'nxdev pack' without '--no-build'.\n";
            }
            return 1;
        }

        if (!json_output) {
            std::cout << "\nNXDevPack - Application Packaging\n\n"
                      << "  Application: " << manifest.application().name << "\n"
                      << "  Format:      " << (is_nsp ? "NSP" : "NRO") << "\n"
                      << "  Profile:     " << profile_name << "\n"
                      << "  ELF:         " << elf_path << " (reusing existing build)\n\n";
        }
    }

    // -------------------------------------------------------------------------
    // Step 2: Package Application
    // -------------------------------------------------------------------------
    pack::PackageRequest req;
    req.manifest = manifest;
    req.project_root = project.root_path();
    req.input_elf_path = elf_path;
    req.output_path = output_path;
    req.keys_path = keys_path;
    req.profile = profile_name;
    req.format = is_nsp ? pack::PackageFormat::NSP : pack::PackageFormat::NRO;
    req.no_build = no_build;
    req.force = force;
    req.dry_run = dry_run;
    req.verbose = verbose;
    req.keep_staging = keep_staging;
    req.debug_backend = debug_backend;
    if (const auto* t = ctx.env.get_tool("elf2nso"); t && t->usable) req.elf2nso_path_override = t->path;
    if (const auto* t = ctx.env.get_tool("npdmtool"); t && t->usable) req.npdmtool_path_override = t->path;
    if (const auto* t = ctx.env.get_tool("nacptool"); t && t->usable) req.nacptool_path_override = t->path;
    if (const auto* t = ctx.env.get_tool("hacbrewpack"); t && t->usable) req.hacbrewpack_path_override = t->path;

    pack::PackManager manager;
    auto progress_cb = [&](pack::PackageStage stage, std::string_view msg) {
        if (!json_output) {
            int stage_num = 2;
            int total_stages = is_nsp ? 7 : 4;

            if (is_nsp) {
                if (stage == pack::PackageStage::PrepareExeFS) stage_num = 2;
                else if (stage == pack::PackageStage::GenerateNPDM) stage_num = 3;
                else if (stage == pack::PackageStage::GenerateNACP) stage_num = 4;
                else if (stage == pack::PackageStage::ResolveAssets) stage_num = 5;
                else if (stage == pack::PackageStage::CreateNSP) stage_num = 6;
                else if (stage == pack::PackageStage::Validate || stage == pack::PackageStage::Finalize) stage_num = 7;
            } else {
                if (stage == pack::PackageStage::GenerateNACP) stage_num = 2;
                else if (stage == pack::PackageStage::ResolveAssets) stage_num = 3;
                else if (stage == pack::PackageStage::CreateNRO || stage == pack::PackageStage::Validate || stage == pack::PackageStage::Finalize) stage_num = 4;
            }

            std::cout << "[" << stage_num << "/" << total_stages << "] " << pack::package_stage_to_string(stage) << ": " << msg << "\n";
        }
    };

    auto pack_res = manager.pack(req, &ctx.env, progress_cb);

    if (json_output) {
        std::cout << pack_res.to_json() << "\n";
        return pack_res.success ? 0 : 1;
    }

    if (!pack_res.success) {
        std::cerr << "\nError [" << pack::pack_error_code_to_string(pack_res.error_code) << "]: "
                  << pack_res.error_message << "\n";
        return pack_res.exit_code != 0 ? pack_res.exit_code : 1;
    }

    std::cout << "\n✔ Packaging completed successfully!\n\n"
              << "Output Artifact:\n"
              << "  " << pack_res.output_file << " (" << pack_res.file_size_bytes << " bytes)\n\n";

    return 0;
}

} // namespace nxdev::cli
