#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace nxdev::env {
class Environment;
}

namespace nxdev::pack {

enum class PackageFormat {
    NRO,
    NSP
};

[[nodiscard]] std::string package_format_to_string(PackageFormat format);
[[nodiscard]] std::optional<PackageFormat> parse_package_format(std::string_view str);

enum class PackageStage {
    Preflight,
    Build,
    GenerateNACP,
    ResolveAssets,
    CreateNRO,
    PrepareExeFS,
    GenerateNPDM,
    CreateNSP,
    Validate,
    Finalize
};

[[nodiscard]] std::string package_stage_to_string(PackageStage stage);

using ProgressCallback = std::function<void(PackageStage stage, std::string_view message)>;

enum class PackErrorCode {
    None = 0,
    Success = 0,
    ToolNotFound,
    InvalidInputBinary,
    InvalidIcon,
    InvalidRomFS,
    RomfsGenerationFailed,
    NacpGenerationFailed,
    MissingTitleId,
    InvalidTitleId,
    InvalidNpdm,
    NsoConversionFailed,
    NpdmGenerationFailed,
    KeyFileMissing,
    PackagingFailed,
    ValidationFailed,
    StaleBuild,
    NotImplemented
};

[[nodiscard]] std::string pack_error_code_to_string(PackErrorCode code);

struct PackageRequest {
    manifest::Manifest manifest;
    std::string project_root;
    std::string input_elf_path;
    std::string output_path;       // If empty, defaults to <project_root>/dist/<profile>/<name>.<ext>
    std::string profile{"debug"};  // "debug" or "release"
    PackageFormat format{PackageFormat::NRO};
    std::string keys_path;         // Path to user-provided prod.keys / keys.dat
    bool no_build{false};
    bool force{false};
    bool dry_run{false};
    bool verbose{false};

    // Tool path overrides (useful for hermetic unit testing)
    std::string nacptool_path_override;
    std::string elf2nro_path_override;
    std::string elf2nso_path_override;
    std::string npdmtool_path_override;
    std::string hacbrewpack_path_override;
    std::string default_icon_path_override;
};

struct PackageResult {
    bool success{false};
    PackageFormat format{PackageFormat::NRO};
    std::string profile{"debug"};
    std::string output_file;
    size_t file_size_bytes{0};

    std::string title_id;
    std::string input_elf_path;
    std::string nacp_file;
    std::string npdm_file;
    std::string nso_file;
    std::string icon_file;
    std::string icon_source; // "project", "libnx_default", "none"
    std::string romfs_dir;
    std::string backend_name;
    std::string backend_version;

    std::vector<std::string> log_messages;
    PackErrorCode error_code{PackErrorCode::None};
    std::string error_message;
    int exit_code{0};

    [[nodiscard]] std::string to_json() const;
};

// Legacy compatibility types for Prompt 1 test compatibility
using PackOptions = PackageRequest;
using PackError = PackageResult;

struct PackOperationResult {
    bool success{false};
    PackageResult result{};

    [[nodiscard]] bool has_value() const noexcept { return success; }
    [[nodiscard]] const PackageResult& value() const noexcept { return result; }
    [[nodiscard]] const PackageResult& error_info() const noexcept { return result; }
    [[nodiscard]] PackErrorCode code() const noexcept { return result.error_code; }
};

/**
 * @brief Abstract backend interface for format-specific packaging (NRO, NSP, etc.)
 */
class IPackBackend {
public:
    virtual ~IPackBackend() = default;

    [[nodiscard]] virtual PackageFormat get_format() const noexcept = 0;
    [[nodiscard]] virtual std::string get_format_name() const noexcept = 0;

    [[nodiscard]] virtual PackageResult pack(
        const PackageRequest& request,
        const env::Environment* env = nullptr,
        ProgressCallback progress = nullptr
    ) = 0;
};

} // namespace nxdev::pack
