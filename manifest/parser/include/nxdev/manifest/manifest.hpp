#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace nxdev::manifest {

// -----------------------------------------------------------------------------
// Application Metadata & Localization
// -----------------------------------------------------------------------------

struct LocalizedAppInfo {
    std::string name;
    std::string author;
    std::string description;
};

struct ApplicationConfig {
    std::string name;
    std::string author;
    std::string version{"1.0.0"};
    std::optional<std::string> title_id;
    std::optional<uint64_t> title_id_numeric;
    std::string description;
    std::map<std::string, LocalizedAppInfo> localized;

    [[nodiscard]] std::string format_title_id() const;
};

// -----------------------------------------------------------------------------
// Assets Configuration
// -----------------------------------------------------------------------------

enum class IconSourceType {
    LibnxDefault,
    ProjectFile
};

struct IconConfig {
    IconSourceType type{IconSourceType::LibnxDefault};
    std::string raw_path;
    std::string resolved_path;
};

struct RomFsConfig {
    bool enabled{false};
    std::string raw_path;
    std::string resolved_path;
};

struct AssetsConfig {
    IconConfig icon;
    RomFsConfig romfs;
};

// -----------------------------------------------------------------------------
// Dependencies Configuration
// -----------------------------------------------------------------------------

struct Dependency {
    std::string name;
    std::string version{"*"};
    bool optional{false};
};

// -----------------------------------------------------------------------------
// Build & Profiles Configuration
// -----------------------------------------------------------------------------

enum class OptimizationLevel {
    Debug,
    Release,
    Size,
    Fast
};

struct BuildProfile {
    OptimizationLevel optimization{OptimizationLevel::Debug};
    bool symbols{true};
    std::vector<std::string> flags;
    std::vector<std::string> defines;
};

struct BuildConfig {
    std::string language{"cpp"};
    std::string standard{"c++20"};
    std::vector<std::string> sources;
    std::vector<std::string> includes;
    std::vector<std::string> defines;
    std::string default_profile{"debug"};
    std::map<std::string, BuildProfile> profiles;
};

// -----------------------------------------------------------------------------
// NACP Configuration
// -----------------------------------------------------------------------------

enum class StartupUserAccount {
    None,
    Required,
    Optional
};

enum class ScreenshotMode {
    Allow,
    Deny
};

enum class VideoCaptureMode {
    Disabled,
    Manual,
    Automatic
};

enum class LogoType {
    LicensedByNintendo,
    Nintendo,
    DistributedByNintendo
};

enum class ParentalControlMode {
    Free,
    Restricted
};

struct NacpConfig {
    std::string display_version{"1.0.0"};
    StartupUserAccount startup_user_account{StartupUserAccount::None};
    bool user_account_switch_lock{false};
    ScreenshotMode screenshot{ScreenshotMode::Allow};
    VideoCaptureMode video_capture{VideoCaptureMode::Disabled};
    LogoType logo_type{LogoType::LicensedByNintendo};
    ParentalControlMode parental_control{ParentalControlMode::Free};
};

// -----------------------------------------------------------------------------
// NPDM Configuration
// -----------------------------------------------------------------------------

enum class NpdmPreset {
    Standard,
    Network,
    Filesystem,
    Multimedia,
    Advanced
};

enum class AddressSpaceType {
    AddressSpace64Bit,
    AddressSpace32Bit,
    AddressSpace32BitNoMap,
    AddressSpace64BitOld
};

struct MainThreadConfig {
    int32_t priority{44};
    int32_t core{0};
    uint32_t stack_size{0x40000}; // 256 KB
};

struct NpdmConfig {
    NpdmPreset preset{NpdmPreset::Standard};
    MainThreadConfig main_thread;
    AddressSpaceType address_space{AddressSpaceType::AddressSpace64Bit};
    std::vector<std::string> services;
    std::vector<std::string> filesystem_permissions;
    std::vector<std::string> kernel_capabilities;
};

// -----------------------------------------------------------------------------
// Packaging Configuration
// -----------------------------------------------------------------------------

enum class PackageFormat {
    NRO,
    NSP
};

struct NroPackagingConfig {
    bool enabled{true};
    bool romfs_enabled{true};
};

struct NspPackagingConfig {
    bool enabled{false};
};

struct PackagingConfig {
    PackageFormat default_format{PackageFormat::NRO};
    NroPackagingConfig nro;
    NspPackagingConfig nsp;
};

// -----------------------------------------------------------------------------
// Development Configuration
// -----------------------------------------------------------------------------

struct NxLinkConfig {
    std::string host;
    uint16_t port{28771};
};

struct DevelopmentConfig {
    NxLinkConfig nxlink;
};

// -----------------------------------------------------------------------------
// Complete Normalized Manifest Model
// -----------------------------------------------------------------------------

class Manifest {
public:
    static constexpr uint32_t SUPPORTED_SCHEMA_VERSION = 1;

    Manifest() = default;
    ~Manifest() = default;

    [[nodiscard]] uint32_t schema_version() const noexcept { return schema_version_; }
    void set_schema_version(uint32_t ver) noexcept { schema_version_ = ver; }

    [[nodiscard]] const std::string& manifest_filepath() const noexcept { return manifest_filepath_; }
    [[nodiscard]] const std::string& project_dir() const noexcept { return project_dir_; }

    void set_manifest_filepath(std::string path) { manifest_filepath_ = std::move(path); }
    void set_project_dir(std::string dir) { project_dir_ = std::move(dir); }

    [[nodiscard]] const ApplicationConfig& application() const noexcept { return application_; }
    [[nodiscard]] ApplicationConfig& application() noexcept { return application_; }

    [[nodiscard]] const AssetsConfig& assets() const noexcept { return assets_; }
    [[nodiscard]] AssetsConfig& assets() noexcept { return assets_; }

    [[nodiscard]] const std::vector<Dependency>& dependencies() const noexcept { return dependencies_; }
    [[nodiscard]] std::vector<Dependency>& dependencies() noexcept { return dependencies_; }

    [[nodiscard]] const BuildConfig& build() const noexcept { return build_; }
    [[nodiscard]] BuildConfig& build() noexcept { return build_; }

    [[nodiscard]] const NacpConfig& nacp() const noexcept { return nacp_; }
    [[nodiscard]] NacpConfig& nacp() noexcept { return nacp_; }

    [[nodiscard]] const NpdmConfig& npdm() const noexcept { return npdm_; }
    [[nodiscard]] NpdmConfig& npdm() noexcept { return npdm_; }

    [[nodiscard]] const PackagingConfig& packaging() const noexcept { return packaging_; }
    [[nodiscard]] PackagingConfig& packaging() noexcept { return packaging_; }

    [[nodiscard]] const DevelopmentConfig& development() const noexcept { return development_; }
    [[nodiscard]] DevelopmentConfig& development() noexcept { return development_; }

    [[nodiscard]] std::string to_human_readable() const;
    [[nodiscard]] std::string to_json() const;

private:
    uint32_t schema_version_{1};
    std::string manifest_filepath_;
    std::string project_dir_;

    ApplicationConfig application_;
    AssetsConfig assets_;
    std::vector<Dependency> dependencies_;
    BuildConfig build_;
    NacpConfig nacp_;
    NpdmConfig npdm_;
    PackagingConfig packaging_;
    DevelopmentConfig development_;
};

} // namespace nxdev::manifest
