#include <nxdev/packages/package_manager.hpp>
#include <nxdev/exec/process.hpp>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>

namespace fs = std::filesystem;

namespace nxdev::packages {

std::string package_status_to_string(PackageStatus status) {
    switch (status) {
        case PackageStatus::Installed: return "installed";
        case PackageStatus::Missing: return "missing";
        case PackageStatus::Incomplete: return "incomplete";
        case PackageStatus::UnsupportedHost: return "unsupported-host";
        case PackageStatus::Unknown: return "unknown";
    }
    return "unknown";
}

std::optional<PackageStatus> parse_package_status(std::string_view str) {
    if (str == "installed") return PackageStatus::Installed;
    if (str == "missing") return PackageStatus::Missing;
    if (str == "incomplete") return PackageStatus::Incomplete;
    if (str == "unsupported-host") return PackageStatus::UnsupportedHost;
    if (str == "unknown") return PackageStatus::Unknown;
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

std::string ModuleStatusInfo::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"id\": \"" << escape_json_str(definition.id) << "\",\n";
    ss << "  \"name\": \"" << escape_json_str(definition.name) << "\",\n";
    ss << "  \"kind\": \"" << package_kind_to_string(definition.kind) << "\",\n";
    ss << "  \"category\": \"" << escape_json_str(definition.category) << "\",\n";
    ss << "  \"status\": \"" << package_status_to_string(status) << "\",\n";
    ss << "  \"version\": \"" << escape_json_str(detected_version) << "\",\n";
    
    ss << "  \"missing_packages\": [";
    for (size_t i = 0; i < missing_system_packages.size(); ++i) {
        ss << "\"" << escape_json_str(missing_system_packages[i]) << "\"" << (i + 1 < missing_system_packages.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"missing_artifacts\": [";
    for (size_t i = 0; i < missing_artifacts.size(); ++i) {
        ss << "\"" << escape_json_str(missing_artifacts[i]) << "\"" << (i + 1 < missing_artifacts.size() ? ", " : "");
    }
    ss << "]\n";

    ss << "}";
    return ss.str();
}

std::string PackageOperationResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"success\": " << (success ? "true" : "false") << ",\n";
    ss << "  \"exit_code\": " << exit_code << ",\n";
    ss << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n";

    ss << "  \"requested_modules\": [";
    for (size_t i = 0; i < requested_modules.size(); ++i) {
        ss << "\"" << escape_json_str(requested_modules[i]) << "\"" << (i + 1 < requested_modules.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"resolved_modules\": [";
    for (size_t i = 0; i < resolved_modules.size(); ++i) {
        ss << "\"" << escape_json_str(resolved_modules[i]) << "\"" << (i + 1 < resolved_modules.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"system_packages_to_install\": [";
    for (size_t i = 0; i < system_packages_to_install.size(); ++i) {
        ss << "\"" << escape_json_str(system_packages_to_install[i]) << "\"" << (i + 1 < system_packages_to_install.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"system_packages_to_remove\": [";
    for (size_t i = 0; i < system_packages_to_remove.size(); ++i) {
        ss << "\"" << escape_json_str(system_packages_to_remove[i]) << "\"" << (i + 1 < system_packages_to_remove.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"command\": [";
    for (size_t i = 0; i < command_executed.size(); ++i) {
        ss << "\"" << escape_json_str(command_executed[i]) << "\"" << (i + 1 < command_executed.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"messages\": [";
    for (size_t i = 0; i < messages.size(); ++i) {
        ss << "\"" << escape_json_str(messages[i]) << "\"" << (i + 1 < messages.size() ? ", " : "");
    }
    ss << "]\n";

    ss << "}";
    return ss.str();
}

// -----------------------------------------------------------------------------
// DevkitProPacmanBackend
// -----------------------------------------------------------------------------

DevkitProPacmanBackend::DevkitProPacmanBackend(const env::Environment& env)
    : env_(env) {
    discover_backend();
}

void DevkitProPacmanBackend::discover_backend() {
    available_ = false;
    pacman_path_.clear();

    // 1. First check if dkp-pacman wrapper is in PATH
    const auto* dkp_pac = env_.get_tool("dkp-pacman");
    if (dkp_pac && dkp_pac->usable) {
        pacman_path_ = dkp_pac->path;
        available_ = true;
        return;
    }

    // 2. Check devkitPro root's pacman/bin/pacman
    if (env_.devkitpro().is_valid) {
        std::string dkp_pacman = env_.devkitpro().path + "/pacman/bin/pacman";
        if (fs::exists(dkp_pacman)) {
            pacman_path_ = dkp_pacman;
            available_ = true;
            return;
        }
    }

    // 3. Fallback to /opt/devkitpro/pacman/bin/pacman if it exists
    if (fs::exists("/opt/devkitpro/pacman/bin/pacman")) {
        pacman_path_ = "/opt/devkitpro/pacman/bin/pacman";
        available_ = true;
        return;
    }
}

bool DevkitProPacmanBackend::is_available() const {
    return available_ && !pacman_path_.empty();
}

std::optional<InstalledPackageInfo> DevkitProPacmanBackend::query_package(const std::string& package_name) {
    if (!is_available()) return std::nullopt;

    auto result = exec::ProcessExecutor::execute(pacman_path_, {"-Q", package_name}, 3000);
    if (!result.success || result.exit_code != 0) {
        return std::nullopt;
    }

    // Parse output: "<package_name> <version>"
    std::istringstream iss(result.stdout_output);
    std::string name, ver;
    if (iss >> name >> ver) {
        return InstalledPackageInfo{
            .package_name = name,
            .version = ver,
            .is_installed = true
        };
    }

    return InstalledPackageInfo{
        .package_name = package_name,
        .version = "",
        .is_installed = true
    };
}

std::unordered_map<std::string, InstalledPackageInfo> DevkitProPacmanBackend::query_all_installed() {
    std::unordered_map<std::string, InstalledPackageInfo> map;
    if (!is_available()) return map;

    auto result = exec::ProcessExecutor::execute(pacman_path_, {"-Q"}, 5000);
    if (!result.success || result.exit_code != 0) {
        return map;
    }

    std::istringstream iss(result.stdout_output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        std::istringstream line_stream(line);
        std::string name, ver;
        if (line_stream >> name >> ver) {
            map[name] = InstalledPackageInfo{
                .package_name = name,
                .version = ver,
                .is_installed = true
            };
        }
    }

    return map;
}

PackageOperationResult DevkitProPacmanBackend::install_packages(
    const std::vector<std::string>& package_names,
    bool dry_run,
    bool /* non_interactive */
) {
    PackageOperationResult res;
    res.system_packages_to_install = package_names;
    res.dry_run = dry_run;

    if (package_names.empty()) {
        res.success = true;
        res.messages.push_back("No system packages to install.");
        return res;
    }

    if (!is_available()) {
        res.success = false;
        res.exit_code = 1;
        res.messages.push_back("devkitPro pacman package manager not found. Please verify your DEVKITPRO installation.");
        return res;
    }

    std::vector<std::string> args = {"-S", "--noconfirm"};
    for (const auto& pkg : package_names) {
        args.push_back(pkg);
    }

    std::vector<std::string> full_cmd = {pacman_path_};
    full_cmd.insert(full_cmd.end(), args.begin(), args.end());
    res.command_executed = full_cmd;

    if (dry_run) {
        res.success = true;
        res.exit_code = 0;
        res.messages.push_back("Dry run: planned installation of " + std::to_string(package_names.size()) + " package(s).");
        return res;
    }

    auto proc_res = exec::ProcessExecutor::execute(pacman_path_, args, 120000);
    res.success = proc_res.success && (proc_res.exit_code == 0);
    res.exit_code = proc_res.exit_code;
    res.stdout_output = proc_res.stdout_output;
    res.stderr_output = proc_res.stderr_output;

    if (!res.success) {
        res.messages.push_back("Package installation failed with exit code " + std::to_string(proc_res.exit_code));
        if (!proc_res.stderr_output.empty()) {
            res.messages.push_back(proc_res.stderr_output);
        }
    } else {
        res.messages.push_back("Successfully installed " + std::to_string(package_names.size()) + " package(s).");
    }

    return res;
}

PackageOperationResult DevkitProPacmanBackend::remove_packages(
    const std::vector<std::string>& package_names,
    bool dry_run,
    bool /* non_interactive */
) {
    PackageOperationResult res;
    res.system_packages_to_remove = package_names;
    res.dry_run = dry_run;

    if (package_names.empty()) {
        res.success = true;
        res.messages.push_back("No system packages to remove.");
        return res;
    }

    if (!is_available()) {
        res.success = false;
        res.exit_code = 1;
        res.messages.push_back("devkitPro pacman package manager not found.");
        return res;
    }

    std::vector<std::string> args = {"-R", "--noconfirm"};
    for (const auto& pkg : package_names) {
        args.push_back(pkg);
    }

    std::vector<std::string> full_cmd = {pacman_path_};
    full_cmd.insert(full_cmd.end(), args.begin(), args.end());
    res.command_executed = full_cmd;

    if (dry_run) {
        res.success = true;
        res.exit_code = 0;
        res.messages.push_back("Dry run: planned removal of " + std::to_string(package_names.size()) + " package(s).");
        return res;
    }

    auto proc_res = exec::ProcessExecutor::execute(pacman_path_, args, 60000);
    res.success = proc_res.success && (proc_res.exit_code == 0);
    res.exit_code = proc_res.exit_code;
    res.stdout_output = proc_res.stdout_output;
    res.stderr_output = proc_res.stderr_output;

    if (!res.success) {
        res.messages.push_back("Package removal failed with exit code " + std::to_string(proc_res.exit_code));
    } else {
        res.messages.push_back("Successfully removed " + std::to_string(package_names.size()) + " package(s).");
    }

    return res;
}

// -----------------------------------------------------------------------------
// MockPackageManagerBackend
// -----------------------------------------------------------------------------

void MockPackageManagerBackend::set_package_installed(const std::string& name, const std::string& version) {
    installed_packages_[name] = InstalledPackageInfo{
        .package_name = name,
        .version = version,
        .is_installed = true
    };
}

void MockPackageManagerBackend::set_package_missing(const std::string& name) {
    installed_packages_.erase(name);
}

void MockPackageManagerBackend::set_operation_should_fail(bool fail, int exit_code, std::string err_msg) {
    should_fail_ = fail;
    failure_exit_code_ = exit_code;
    failure_error_msg_ = std::move(err_msg);
}

std::optional<InstalledPackageInfo> MockPackageManagerBackend::query_package(const std::string& package_name) {
    if (!available_) return std::nullopt;
    auto it = installed_packages_.find(package_name);
    if (it != installed_packages_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::unordered_map<std::string, InstalledPackageInfo> MockPackageManagerBackend::query_all_installed() {
    if (!available_) return {};
    return installed_packages_;
}

PackageOperationResult MockPackageManagerBackend::install_packages(
    const std::vector<std::string>& package_names,
    bool dry_run,
    bool /* non_interactive */
) {
    PackageOperationResult res;
    res.system_packages_to_install = package_names;
    res.dry_run = dry_run;

    std::vector<std::string> cmd = {executable_path(), "-S", "--noconfirm"};
    cmd.insert(cmd.end(), package_names.begin(), package_names.end());
    res.command_executed = cmd;

    if (!available_) {
        res.success = false;
        res.exit_code = 1;
        res.messages.push_back("Mock package manager unavailable");
        return res;
    }

    if (should_fail_) {
        res.success = false;
        res.exit_code = failure_exit_code_;
        res.stderr_output = failure_error_msg_;
        res.messages.push_back("Mock failure: " + failure_error_msg_);
        return res;
    }

    if (!dry_run) {
        for (const auto& pkg : package_names) {
            set_package_installed(pkg, "1.0.0-mock");
        }
    }

    res.success = true;
    res.exit_code = 0;
    res.messages.push_back(dry_run ? "Mock dry-run install succeeded" : "Mock install succeeded");
    return res;
}

PackageOperationResult MockPackageManagerBackend::remove_packages(
    const std::vector<std::string>& package_names,
    bool dry_run,
    bool /* non_interactive */
) {
    PackageOperationResult res;
    res.system_packages_to_remove = package_names;
    res.dry_run = dry_run;

    std::vector<std::string> cmd = {executable_path(), "-R", "--noconfirm"};
    cmd.insert(cmd.end(), package_names.begin(), package_names.end());
    res.command_executed = cmd;

    if (!available_) {
        res.success = false;
        res.exit_code = 1;
        res.messages.push_back("Mock package manager unavailable");
        return res;
    }

    if (should_fail_) {
        res.success = false;
        res.exit_code = failure_exit_code_;
        res.stderr_output = failure_error_msg_;
        res.messages.push_back("Mock failure: " + failure_error_msg_);
        return res;
    }

    if (!dry_run) {
        for (const auto& pkg : package_names) {
            set_package_missing(pkg);
        }
    }

    res.success = true;
    res.exit_code = 0;
    res.messages.push_back(dry_run ? "Mock dry-run remove succeeded" : "Mock remove succeeded");
    return res;
}

// -----------------------------------------------------------------------------
// PackageManager Coordinator
// -----------------------------------------------------------------------------

PackageManager::PackageManager(
    const env::Environment& env,
    const PackageRegistry& registry,
    std::unique_ptr<IPackageManagerBackend> backend
) : env_(env),
    registry_(registry),
    backend_(backend ? std::move(backend) : std::make_unique<DevkitProPacmanBackend>(env)),
    resolver_(registry) {}

bool PackageManager::check_artifact_presence(
    const PackageDefinition& pkg,
    std::vector<std::string>& out_missing_artifacts
) const {
    if (pkg.is_builtin()) return true;

    bool all_present = true;
    std::string dkp_root = env_.devkitpro().path;
    if (dkp_root.empty()) dkp_root = "/opt/devkitpro";

    std::string portlibs_inc = dkp_root + "/portlibs/switch/include";
    std::string portlibs_lib = dkp_root + "/portlibs/switch/lib";
    std::string libnx_inc = dkp_root + "/libnx/include";
    std::string libnx_lib = dkp_root + "/libnx/lib";

    for (const auto& hdr : pkg.cmake.headers) {
        std::string p1 = portlibs_inc + "/" + hdr;
        std::string p2 = libnx_inc + "/" + hdr;
        if (!fs::exists(p1) && !fs::exists(p2)) {
            out_missing_artifacts.push_back("Header missing: " + hdr);
            all_present = false;
        }
    }

    for (const auto& lib : pkg.cmake.libraries) {
        std::string p1 = portlibs_lib + "/" + lib;
        std::string p2 = libnx_lib + "/" + lib;
        if (!fs::exists(p1) && !fs::exists(p2)) {
            out_missing_artifacts.push_back("Library missing: " + lib);
            all_present = false;
        }
    }

    return all_present;
}

ModuleStatusInfo PackageManager::check_module_status(const PackageDefinition& pkg) const {
    ModuleStatusInfo info;
    info.definition = pkg;

    if (pkg.is_builtin()) {
        info.status = PackageStatus::Installed;
        info.detected_version = "built-in";
        return info;
    }

    if (!backend_->is_available()) {
        // If package manager is not available, try checking disk artifacts directly
        std::vector<std::string> missing_art;
        if (check_artifact_presence(pkg, missing_art)) {
            info.status = PackageStatus::Installed;
            info.detected_version = "manual";
        } else {
            info.status = PackageStatus::UnsupportedHost;
            info.missing_system_packages = pkg.devkitpro.packages;
            info.missing_artifacts = std::move(missing_art);
        }
        return info;
    }

    // Query package manager
    bool all_pkgs_installed = true;
    std::string version_str;
    for (const auto& sys_pkg : pkg.devkitpro.packages) {
        auto query_res = backend_->query_package(sys_pkg);
        if (query_res.has_value() && query_res->is_installed) {
            if (version_str.empty()) version_str = query_res->version;
        } else {
            all_pkgs_installed = false;
            info.missing_system_packages.push_back(sys_pkg);
        }
    }

    if (!all_pkgs_installed) {
        info.status = PackageStatus::Missing;
        info.detected_version = "";
        return info;
    }

    // Verify artifacts if not mock backend
    bool is_mock = (backend_->backend_name().find("ock") != std::string::npos);
    if (!is_mock) {
        std::vector<std::string> missing_art;
        bool artifacts_present = check_artifact_presence(pkg, missing_art);

        if (!artifacts_present && !missing_art.empty()) {
            info.status = PackageStatus::Incomplete;
            info.detected_version = version_str;
            info.missing_artifacts = std::move(missing_art);
        } else {
            info.status = PackageStatus::Installed;
            info.detected_version = version_str;
        }
    } else {
        info.status = PackageStatus::Installed;
        info.detected_version = version_str;
    }

    return info;
}

std::vector<ModuleStatusInfo> PackageManager::check_all_statuses() const {
    std::vector<ModuleStatusInfo> results;
    for (const auto& pkg : registry_.packages()) {
        results.push_back(check_module_status(pkg));
    }
    return results;
}

std::vector<ModuleStatusInfo> PackageManager::check_manifest_dependencies(const manifest::Manifest& manifest) const {
    auto resolution = resolver_.resolve_manifest(manifest);
    std::vector<ModuleStatusInfo> results;

    for (const auto& pkg_def : resolution.resolved_packages) {
        results.push_back(check_module_status(pkg_def));
    }

    return results;
}

PackageOperationResult PackageManager::install(
    const std::vector<std::string>& requested_ids,
    bool dry_run,
    bool non_interactive
) {
    PackageOperationResult res;
    res.requested_modules = requested_ids;
    res.dry_run = dry_run;

    auto resolution = resolver_.resolve(requested_ids);
    if (!resolution.success) {
        res.success = false;
        res.exit_code = 1;
        res.messages = resolution.errors;
        return res;
    }

    res.resolved_modules = resolution.ordered_module_ids;

    // Filter out packages that are already installed
    std::vector<std::string> to_install;
    for (const auto& pkg_def : resolution.resolved_packages) {
        if (pkg_def.is_builtin()) continue;

        for (const auto& sys_pkg : pkg_def.devkitpro.packages) {
            auto q = backend_->query_package(sys_pkg);
            if (!q.has_value() || !q->is_installed) {
                if (std::find(to_install.begin(), to_install.end(), sys_pkg) == to_install.end()) {
                    to_install.push_back(sys_pkg);
                }
            }
        }
    }

    res.system_packages_to_install = to_install;

    if (to_install.empty()) {
        res.success = true;
        res.exit_code = 0;
        res.messages.push_back("All requested dependencies are already installed and up to date.");
        return res;
    }

    return backend_->install_packages(to_install, dry_run, non_interactive);
}

PackageOperationResult PackageManager::install_missing(
    const manifest::Manifest& manifest,
    bool dry_run,
    bool non_interactive
) {
    std::vector<std::string> dep_names;
    for (const auto& dep : manifest.dependencies()) {
        dep_names.push_back(dep.name);
    }
    return install(dep_names, dry_run, non_interactive);
}

PackageOperationResult PackageManager::remove(
    const std::vector<std::string>& requested_ids,
    bool dry_run,
    bool non_interactive
) {
    PackageOperationResult res;
    res.requested_modules = requested_ids;
    res.dry_run = dry_run;

    std::vector<std::string> to_remove;
    for (const auto& id : requested_ids) {
        const auto* pkg = registry_.find(id);
        if (!pkg) {
            res.success = false;
            res.exit_code = 1;
            res.messages.push_back("Unknown module: " + id);
            return res;
        }

        if (pkg->is_builtin()) {
            res.success = false;
            res.exit_code = 1;
            res.messages.push_back("Cannot remove built-in module '" + pkg->id + "'");
            return res;
        }

        for (const auto& sys_pkg : pkg->devkitpro.packages) {
            if (std::find(to_remove.begin(), to_remove.end(), sys_pkg) == to_remove.end()) {
                to_remove.push_back(sys_pkg);
            }
        }
    }

    res.system_packages_to_remove = to_remove;

    if (to_remove.empty()) {
        res.success = true;
        res.exit_code = 0;
        res.messages.push_back("No underlying devkitPro packages found to remove.");
        return res;
    }

    return backend_->remove_packages(to_remove, dry_run, non_interactive);
}

} // namespace nxdev::packages
