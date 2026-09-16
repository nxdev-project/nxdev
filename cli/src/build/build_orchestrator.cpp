#include <nxdev/build/build_orchestrator.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/exec/resource_calculator.hpp>
#include <nxdev/exec/controlled_runner.hpp>
#include <nxdev/packages/registry.hpp>
#include <nxdev/packages/package_manager.hpp>
#include <nxdev/packages/resolver.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <thread>

namespace nxdev::build {

namespace fs = std::filesystem;

std::string build_profile_to_string(BuildProfile profile) {
    switch (profile) {
        case BuildProfile::Debug: return "debug";
        case BuildProfile::Release: return "release";
    }
    return "debug";
}

std::optional<BuildProfile> parse_build_profile(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower == "debug") return BuildProfile::Debug;
    if (lower == "release") return BuildProfile::Release;
    return std::nullopt;
}

std::string BuildResult::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"status\": \"" << (success ? "success" : "error") << "\",\n";
    oss << "  \"exitCode\": " << exit_code << ",\n";
    oss << "  \"profile\": \"" << profile << "\",\n";
    oss << "  \"projectRoot\": \"" << project_root << "\",\n";
    oss << "  \"buildDirectory\": \"" << build_directory << "\",\n";
    oss << "  \"targetName\": \"" << target_name << "\",\n";
    oss << "  \"artifact\": \"" << elf_path << "\",\n";
    oss << "  \"compileCommands\": \"" << compile_commands_path << "\",\n";
    oss << "  \"type\": \"elf\",\n";
    oss << "  \"peakMemoryBytes\": " << peak_memory_bytes << ",\n";
    oss << "  \"durationMs\": " << duration_ms << ",\n";
    oss << "  \"jobs\": " << parallel_jobs << ",\n";
    oss << "  \"resourceController\": \"" << resource_controller << "\",\n";
    oss << "  \"resourceLimited\": " << (resource_limit_exceeded ? "true" : "false") << ",\n";
    oss << "  \"diagnostics\": [\n";
    for (size_t i = 0; i < diagnostics.size(); ++i) {
        oss << "    \"" << diagnostics[i] << "\"" << (i + 1 < diagnostics.size() ? "," : "") << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

std::string BuildOrchestrator::resolve_build_directory(
    const std::string& project_root,
    const std::string& profile_name
) {
    return (fs::path(project_root) / ".nxdev" / "build" / profile_name).string();
}

static fs::path find_cmake_toolchain_dir() {
#ifdef NXDEV_SOURCE_DIR
    fs::path p = fs::path(NXDEV_SOURCE_DIR) / "cmake";
    if (fs::exists(p / "toolchains" / "NXDevSwitch.cmake")) {
        return p;
    }
#endif
    // Fallback relative to current working dir or common paths
    if (fs::exists("cmake/toolchains/NXDevSwitch.cmake")) {
        return fs::absolute("cmake");
    }
    if (fs::exists("../cmake/toolchains/NXDevSwitch.cmake")) {
        return fs::canonical("../cmake");
    }
    return fs::path("/usr/local/share/nxdev/cmake");
}

std::vector<std::string> BuildOrchestrator::generate_configure_args(
    const project::NXDevProject& project,
    const env::Environment& env,
    const BuildOptions& options,
    const std::string& build_dir
) {
    std::vector<std::string> args;
    args.push_back("-S");
    args.push_back(project.root_path());
    args.push_back("-B");
    args.push_back(build_dir);

    // Generator selection
    if (options.generator.has_value() && !options.generator->empty()) {
        args.push_back("-G");
        args.push_back(*options.generator);
    } else {
        const auto* ninja_tool = env.get_tool("ninja");
        if (ninja_tool && ninja_tool->usable) {
            args.push_back("-G");
            args.push_back("Ninja");
        } else {
            args.push_back("-G");
            args.push_back("Unix Makefiles");
        }
    }

    // Toolchain file
    fs::path cmake_dir = find_cmake_toolchain_dir();
    fs::path toolchain_file = cmake_dir / "toolchains" / "NXDevSwitch.cmake";
    args.push_back("-DCMAKE_TOOLCHAIN_FILE=" + toolchain_file.string());

    // devkitPro / devkitA64 paths
    if (!env.devkitpro().path.empty()) {
        args.push_back("-DNXDEV_DEVKITPRO=" + env.devkitpro().path);
    }
    if (!env.devkita64().path.empty()) {
        args.push_back("-DNXDEV_DEVKITA64=" + env.devkita64().path);
    }

    // Package and module paths for find_package(NXDev)
    args.push_back("-DNXDev_DIR=" + cmake_dir.string());
    args.push_back("-DCMAKE_PREFIX_PATH=" + cmake_dir.parent_path().string() + ";" + cmake_dir.string());
    args.push_back("-DCMAKE_MODULE_PATH=" + cmake_dir.string());

    // Build configuration
    std::string build_type = (options.profile == BuildProfile::Release) ? "Release" : "Debug";
    args.push_back("-DCMAKE_BUILD_TYPE=" + build_type);

    // Export compile commands for IDE tooling
    args.push_back("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON");

    // Standard from manifest if available
    if (project.is_valid()) {
        const auto& b = project.manifest().build();
        if (b.standard.find("c++20") != std::string::npos || b.standard.find("20") != std::string::npos) {
            args.push_back("-DCMAKE_CXX_STANDARD=20");
        } else if (b.standard.find("c++17") != std::string::npos || b.standard.find("17") != std::string::npos) {
            args.push_back("-DCMAKE_CXX_STANDARD=17");
        } else if (b.standard.find("c++23") != std::string::npos || b.standard.find("23") != std::string::npos) {
            args.push_back("-DCMAKE_CXX_STANDARD=23");
        }
    }

    // Append any extra CMake arguments
    for (const auto& extra : options.extra_cmake_args) {
        args.push_back(extra);
    }

    return args;
}

bool BuildOrchestrator::safe_clean(const project::NXDevProject& project, std::string& error_message) {
    if (project.root_path().empty()) {
        error_message = "Project root path is empty.";
        return false;
    }

    try {
        fs::path root = fs::canonical(project.root_path());
        fs::path build_dir = root / ".nxdev" / "build";

        // Safety verification: Ensure target is within project root and is specifically .nxdev/build
        if (root == "/" || root == fs::path("/") || root == fs::temp_directory_path()) {
            error_message = "Refusing to clean: project root is a protected filesystem path: " + root.string();
            return false;
        }

        if (fs::exists(build_dir)) {
            fs::path canonical_build = fs::canonical(build_dir);
            // Verify parent path is .nxdev inside root
            if (canonical_build.parent_path() != (root / ".nxdev")) {
                error_message = "Refusing to clean: build directory does not reside safely in .nxdev";
                return false;
            }
            fs::remove_all(build_dir);
        }

        // Clean top-level compile_commands pointer if present
        fs::path cc = root / ".nxdev" / "compile_commands.json";
        if (fs::exists(cc)) {
            fs::remove(cc);
        }

        return true;
    } catch (const std::exception& e) {
        error_message = std::string("Clean failed with filesystem exception: ") + e.what();
        return false;
    }
}

BuildResult BuildOrchestrator::configure(
    const project::NXDevProject& project,
    const env::Environment& env,
    const BuildOptions& options
) {
    BuildResult result;
    result.project_root = project.root_path();
    result.profile = build_profile_to_string(options.profile);
    result.build_directory = resolve_build_directory(project.root_path(), result.profile);

    // Verify prerequisites
    if (!env.devkita64().is_valid && !env.devkitpro().is_valid) {
        result.success = false;
        result.exit_code = 1;
        result.diagnostics.push_back("devkitPro / devkitA64 toolchain is missing or invalid. Run 'nxdev doctor' to inspect.");
        return result;
    }

    // Verify declared dependencies before executing CMake
    if (project.is_valid() && !project.manifest().dependencies().empty()) {
        auto reg_res = packages::PackageRegistry::load_default(env);
        if (reg_res.is_success()) {
            packages::PackageManager pm(env, reg_res.value());
            auto statuses = pm.check_manifest_dependencies(project.manifest());
            for (const auto& s : statuses) {
                if (s.status == packages::PackageStatus::Missing) {
                    result.success = false;
                    result.exit_code = 1;
                    std::string pkg_name = s.missing_system_packages.empty() ? s.definition.id : s.missing_system_packages[0];
                    result.diagnostics.push_back("Missing required project dependency: '" + s.definition.id + "' (requires " + pkg_name + ")");
                    result.diagnostics.push_back("Install with: 'nxdev package install " + s.definition.id + "' or 'nxdev package install --missing'");
                    return result;
                } else if (s.status == packages::PackageStatus::Incomplete) {
                    result.success = false;
                    result.exit_code = 1;
                    result.diagnostics.push_back("Incomplete dependency installation: '" + s.definition.id + "' (missing header or library files)");
                    result.diagnostics.push_back("Reinstall with: 'nxdev package install " + s.definition.id + "'");
                    return result;
                }
            }
        }
    }

    if (options.fresh_configure) {
        if (fs::exists(result.build_directory)) {
            fs::remove_all(result.build_directory);
        }
    }

    fs::create_directories(result.build_directory);

    auto configure_args = generate_configure_args(project, env, options, result.build_directory);

    auto exec_res = exec::ProcessExecutor::execute("cmake", configure_args, 60000, project.root_path());
    result.exit_code = exec_res.exit_code;
    result.stdout_output = exec_res.stdout_output;
    result.stderr_output = exec_res.stderr_output;
    result.success = exec_res.success && (exec_res.exit_code == 0);

    if (!result.success) {
        result.diagnostics.push_back("CMake configuration failed with exit code " + std::to_string(exec_res.exit_code));
    } else {
        // Write dependency fingerprint
        if (project.is_valid()) {
            std::ofstream fp(fs::path(result.build_directory) / "dependencies.fingerprint");
            for (const auto& d : project.manifest().dependencies()) {
                fp << d.name << "\n";
            }
        }
    }

    return result;
}

BuildResult BuildOrchestrator::build(
    const project::NXDevProject& project,
    const env::Environment& env,
    const BuildOptions& options
) {
    BuildResult result;
    result.project_root = project.root_path();
    result.profile = build_profile_to_string(options.profile);
    result.build_directory = resolve_build_directory(project.root_path(), result.profile);

    // 1. Clean first if requested
    if (options.clean_first) {
        std::string err;
        safe_clean(project, err);
    }

    // Check if dependency fingerprint changed
    bool deps_changed = false;
    if (project.is_valid()) {
        fs::path fp_path = fs::path(result.build_directory) / "dependencies.fingerprint";
        if (fs::exists(fp_path)) {
            std::ifstream fp(fp_path);
            std::string line;
            std::vector<std::string> saved_deps;
            while (std::getline(fp, line)) {
                if (!line.empty()) saved_deps.push_back(line);
            }
            std::vector<std::string> cur_deps;
            for (const auto& d : project.manifest().dependencies()) cur_deps.push_back(d.name);
            if (saved_deps != cur_deps) {
                deps_changed = true;
            }
        }
    }

    // 2. Configure if CMakeCache does not exist, fresh requested, or dependencies changed
    bool needs_configure = options.fresh_configure || deps_changed || !fs::exists(fs::path(result.build_directory) / "CMakeCache.txt");
    if (needs_configure) {
        auto config_res = configure(project, env, options);
        if (!config_res.success) {
            return config_res;
        }
    }

    // 3. Compute Resource Budget and Safe Job Parallelism
    exec::MemoryInfo mem = exec::ResourceCalculator::detect_memory();
    size_t host_reserve = exec::ResourceCalculator::compute_host_reserve(mem);
    size_t safe_budget = exec::ResourceCalculator::compute_safe_memory_budget(
        exec::WorkloadType::BuildApplication,
        mem,
        options.max_memory_bytes
    );

    if (safe_budget < exec::ResourceCalculator::MINIMUM_VIABLE_BUILD_BUDGET && !options.unsafe_no_limits) {
        result.success = false;
        result.exit_code = 1;
        result.resource_limit_exceeded = true;
        result.resource_limit_reason = "Insufficient host memory to launch build safely";
        result.diagnostics.push_back(
            "Not enough safe memory is currently available to build application. Available: " +
            exec::format_bytes(mem.available_bytes) + ", Reserved for host/WSL: " +
            exec::format_bytes(host_reserve) + ", Safe build budget: " +
            exec::format_bytes(safe_budget) + ". NXDev will not start the compiler because it could destabilize the system."
        );
        return result;
    }

    bool was_clamped = false;
    uint32_t safe_jobs = exec::ResourceCalculator::compute_safe_job_count(
        exec::WorkloadType::BuildApplication,
        mem,
        safe_budget,
        options.parallel_jobs,
        options.unsafe_no_limits,
        &was_clamped
    );

    result.parallel_jobs = safe_jobs;

    // Sanitize environment variables against unsafe hidden -j flags
    exec::ResourceCalculator::sanitize_environment(safe_jobs);

    if (options.verbose) {
        uint32_t cpus = std::thread::hardware_concurrency();
        std::cout << "\nBuild resource policy:\n"
                  << "  Platform:            " << (mem.is_wsl ? "WSL2" : "Linux / Host") << "\n"
                  << "  CPUs visible:        " << cpus << "\n"
                  << "  Memory visible:      " << exec::format_bytes(mem.total_bytes) << "\n"
                  << "  Memory available:    " << exec::format_bytes(mem.available_bytes) << "\n"
                  << "  Reserved for host:   " << exec::format_bytes(host_reserve) << "\n"
                  << "  Build memory limit:  " << exec::format_bytes(safe_budget) << "\n"
                  << "  Compile jobs:        " << safe_jobs << (was_clamped ? " (clamped from requested)" : "") << "\n"
                  << "  Link jobs:           1\n\n";
    }

    // 4. Setup Resource Policy and Invoke Controlled cmake --build
    exec::ProcessResourcePolicy proc_policy;
    proc_policy.workload = exec::WorkloadType::BuildApplication;
    proc_policy.max_memory_bytes = safe_budget;
    proc_policy.min_host_memory_reserve_bytes = host_reserve;
    proc_policy.timeout_ms = 600000;
    proc_policy.working_dir = project.root_path();
    proc_policy.unsafe_no_limits = options.unsafe_no_limits;

    fs::path log_file = fs::path(result.build_directory) / "build.log";
    fs::path stdout_log = fs::path(result.build_directory) / "build.stdout.log";
    fs::path stderr_log = fs::path(result.build_directory) / "build.stderr.log";
    proc_policy.log_file_path = log_file.string();
    proc_policy.stdout_log_path = stdout_log.string();
    proc_policy.stderr_log_path = stderr_log.string();

    std::vector<std::string> build_args = {"--build", result.build_directory};
    build_args.push_back("--parallel");
    build_args.push_back(std::to_string(safe_jobs));

    if (options.verbose) {
        build_args.push_back("--verbose");
    }

    auto exec_res = exec::ControlledProcessRunner::execute("cmake", build_args, proc_policy);

    result.exit_code = exec_res.exit_code;
    result.stdout_output = exec_res.stdout_output;
    result.stderr_output = exec_res.stderr_output;
    result.peak_memory_bytes = exec_res.peak_rss_bytes;
    result.duration_ms = exec_res.duration_ms;
    result.resource_controller = exec::controller_type_to_string(exec_res.controller);
    result.resource_limit_exceeded = exec_res.resource_limit_exceeded;
    result.resource_limit_reason = exec::resource_limit_reason_to_string(exec_res.resource_limit_reason);
    result.success = exec_res.success && (exec_res.exit_code == 0);

    if (!result.success) {
        if (exec_res.resource_limit_exceeded) {
            result.diagnostics.push_back(
                "Build stopped by NXDev resource controller. Reason: " +
                result.resource_limit_reason + " (Peak RSS: " + exec::format_bytes(exec_res.peak_rss_bytes) +
                ", Configured limit: " + exec::format_bytes(safe_budget) + ")."
            );
        } else {
            result.diagnostics.push_back("CMake build step failed with exit code " + std::to_string(exec_res.exit_code));
        }
        return result;
    }

    // 5. Locate primary target ELF
    std::string app_id = "app";
    if (project.is_valid()) {
        const auto& app = project.manifest().application();
        if (app.title_id.has_value() && !app.title_id->empty()) {
            app_id = *app.title_id;
        } else if (!app.name.empty()) {
            std::string n = app.name;
            std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) {
                return (std::isalnum(c) || c == '-') ? std::tolower(c) : '_';
            });
            app_id = n;
        }
    }
    result.target_name = app_id;

    // Search for produced ELF in bin/ and build directory
    fs::path bin_dir = fs::path(result.build_directory) / "bin";
    if (fs::exists(bin_dir)) {
        for (const auto& entry : fs::directory_iterator(bin_dir)) {
            if (entry.path().extension() == ".elf") {
                result.elf_path = fs::canonical(entry.path()).string();
                result.target_name = entry.path().stem().string();
                break;
            }
        }
    }
    if (result.elf_path.empty() && fs::exists(result.build_directory)) {
        for (const auto& entry : fs::directory_iterator(result.build_directory)) {
            if (entry.path().extension() == ".elf") {
                result.elf_path = fs::canonical(entry.path()).string();
                result.target_name = entry.path().stem().string();
                break;
            }
        }
    }

    // 6. Handle compile_commands.json
    fs::path src_cc = fs::path(result.build_directory) / "compile_commands.json";
    if (fs::exists(src_cc)) {
        result.compile_commands_path = fs::canonical(src_cc).string();
        fs::path dest_cc = fs::path(project.root_path()) / ".nxdev" / "compile_commands.json";
        std::error_code ec;
        fs::copy_file(src_cc, dest_cc, fs::copy_options::overwrite_existing, ec);
    }

    // 7. Write build-info.json
    fs::path build_info_file = fs::path(result.build_directory) / "build-info.json";
    std::ofstream binfo(build_info_file);
    if (binfo.is_open()) {
        binfo << result.to_json();
    }

    return result;
}

} // namespace nxdev::build
