#pragma once

#include <nxdev/packages/registry.hpp>
#include <nxdev/packages/resolver.hpp>
#include <nxdev/env/environment.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace nxdev::packages {

enum class PackageStatus {
    Installed,
    Missing,
    Incomplete,     // Package manager reports installed, but expected headers/libraries are missing
    UnsupportedHost,
    Unknown
};

std::string package_status_to_string(PackageStatus status);
std::optional<PackageStatus> parse_package_status(std::string_view str);

struct InstalledPackageInfo {
    std::string package_name;
    std::string version;
    bool is_installed{false};
};

struct ModuleStatusInfo {
    PackageDefinition definition;
    PackageStatus status{PackageStatus::Unknown};
    std::string detected_version;
    std::vector<std::string> missing_system_packages;
    std::vector<std::string> missing_artifacts;

    [[nodiscard]] std::string to_json() const;
};

struct PackageOperationResult {
    bool success{false};
    int exit_code{0};
    bool dry_run{false};
    std::vector<std::string> requested_modules;
    std::vector<std::string> resolved_modules;
    std::vector<std::string> system_packages_to_install;
    std::vector<std::string> system_packages_to_remove;
    std::vector<std::string> command_executed;
    std::string stdout_output;
    std::string stderr_output;
    std::vector<std::string> messages;

    [[nodiscard]] std::string to_json() const;
};

/**
 * @brief Abstract interface for underlying package manager backends.
 */
class IPackageManagerBackend {
public:
    virtual ~IPackageManagerBackend() = default;

    [[nodiscard]] virtual bool is_available() const = 0;
    [[nodiscard]] virtual std::string executable_path() const = 0;
    [[nodiscard]] virtual std::string backend_name() const = 0;

    virtual std::optional<InstalledPackageInfo> query_package(const std::string& package_name) = 0;
    virtual std::unordered_map<std::string, InstalledPackageInfo> query_all_installed() = 0;

    virtual PackageOperationResult install_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) = 0;

    virtual PackageOperationResult remove_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) = 0;
};

/**
 * @brief Concrete backend interfacing with devkitPro's pacman (dkp-pacman).
 */
class DevkitProPacmanBackend : public IPackageManagerBackend {
public:
    explicit DevkitProPacmanBackend(const env::Environment& env);
    ~DevkitProPacmanBackend() override = default;

    [[nodiscard]] bool is_available() const override;
    [[nodiscard]] std::string executable_path() const override { return pacman_path_; }
    [[nodiscard]] std::string backend_name() const override { return "devkitPro pacman"; }

    std::optional<InstalledPackageInfo> query_package(const std::string& package_name) override;
    std::unordered_map<std::string, InstalledPackageInfo> query_all_installed() override;

    PackageOperationResult install_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) override;

    PackageOperationResult remove_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) override;

private:
    const env::Environment& env_;
    std::string pacman_path_;
    bool available_{false};

    void discover_backend();
};

/**
 * @brief Mockable package manager backend for hermetic unit and integration testing.
 */
class MockPackageManagerBackend : public IPackageManagerBackend {
public:
    MockPackageManagerBackend() = default;
    ~MockPackageManagerBackend() override = default;

    [[nodiscard]] bool is_available() const override { return available_; }
    [[nodiscard]] std::string executable_path() const override { return "/mock/bin/dkp-pacman"; }
    [[nodiscard]] std::string backend_name() const override { return "Mock pacman"; }

    void set_available(bool avail) { available_ = avail; }
    void set_package_installed(const std::string& name, const std::string& version = "1.0.0");
    void set_package_missing(const std::string& name);
    void set_operation_should_fail(bool fail, int exit_code = 1, std::string err_msg = "Mock error");

    std::optional<InstalledPackageInfo> query_package(const std::string& package_name) override;
    std::unordered_map<std::string, InstalledPackageInfo> query_all_installed() override;

    PackageOperationResult install_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) override;

    PackageOperationResult remove_packages(
        const std::vector<std::string>& package_names,
        bool dry_run,
        bool non_interactive
    ) override;

private:
    bool available_{true};
    bool should_fail_{false};
    int failure_exit_code_{1};
    std::string failure_error_msg_;
    std::unordered_map<std::string, InstalledPackageInfo> installed_packages_;
};

/**
 * @brief High-level package manager coordinator in NXDev.
 */
class PackageManager {
public:
    PackageManager(
        const env::Environment& env,
        const PackageRegistry& registry,
        std::unique_ptr<IPackageManagerBackend> backend = nullptr
    );
    ~PackageManager() = default;

    [[nodiscard]] const IPackageManagerBackend& backend() const noexcept { return *backend_; }
    [[nodiscard]] const PackageRegistry& registry() const noexcept { return registry_; }

    /**
     * @brief Checks installation and integrity status for a single module.
     */
    [[nodiscard]] ModuleStatusInfo check_module_status(const PackageDefinition& pkg) const;

    /**
     * @brief Checks installation and integrity status for all modules in registry.
     */
    [[nodiscard]] std::vector<ModuleStatusInfo> check_all_statuses() const;

    /**
     * @brief Checks installation status for dependencies declared in a project manifest.
     */
    [[nodiscard]] std::vector<ModuleStatusInfo> check_manifest_dependencies(const manifest::Manifest& manifest) const;

    /**
     * @brief Installs requested NXDev modules, resolving dependencies first.
     */
    [[nodiscard]] PackageOperationResult install(
        const std::vector<std::string>& requested_ids,
        bool dry_run = false,
        bool non_interactive = false
    );

    /**
     * @brief Installs any missing dependencies declared in a project manifest.
     */
    [[nodiscard]] PackageOperationResult install_missing(
        const manifest::Manifest& manifest,
        bool dry_run = false,
        bool non_interactive = false
    );

    /**
     * @brief Removes specified NXDev modules conservatively.
     */
    [[nodiscard]] PackageOperationResult remove(
        const std::vector<std::string>& requested_ids,
        bool dry_run = false,
        bool non_interactive = false
    );

private:
    const env::Environment& env_;
    const PackageRegistry& registry_;
    std::unique_ptr<IPackageManagerBackend> backend_;
    DependencyResolver resolver_;

    [[nodiscard]] bool check_artifact_presence(
        const PackageDefinition& pkg,
        std::vector<std::string>& out_missing_artifacts
    ) const;
};

} // namespace nxdev::packages
