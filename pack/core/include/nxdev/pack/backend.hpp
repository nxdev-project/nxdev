#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <string>
#include <memory>

namespace nxdev::pack {

enum class PackageFormat {
    NRO,
    NSP
};

enum class PackErrorCode {
    None,
    Success,
    ToolNotFound,
    InvalidInputBinary,
    RomfsGenerationFailed,
    NacpGenerationFailed,
    KeyFileMissing,
    PackagingFailed,
    NotImplemented
};

struct PackError {
    PackErrorCode code{PackErrorCode::PackagingFailed};
    std::string message;

    [[nodiscard]] bool has_error() const noexcept {
        return code != PackErrorCode::None && code != PackErrorCode::Success;
    }
};

struct PackOptions {
    PackageFormat format{PackageFormat::NRO};
    std::string input_elf_path;
    std::string output_path;
    std::string keys_file_path; // Only used when required by format (e.g. NSP)
    bool verbose{false};
};

struct PackResult {
    bool success{false};
    std::string output_file;
    size_t file_size_bytes{0};
};

struct PackOperationResult {
    bool success{false};
    PackResult result{};
    PackError error{};

    [[nodiscard]] bool has_value() const noexcept { return success; }
    [[nodiscard]] const PackResult& value() const noexcept { return result; }
    [[nodiscard]] const PackError& error_info() const noexcept { return error; }
};

/**
 * @brief Abstract backend interface for format-specific packaging (NRO, NSP, etc.)
 */
class IPackBackend {
public:
    virtual ~IPackBackend() = default;

    [[nodiscard]] virtual PackageFormat get_format() const noexcept = 0;
    [[nodiscard]] virtual std::string get_format_name() const noexcept = 0;
    [[nodiscard]] virtual PackOperationResult pack(
        const manifest::Manifest& manifest,
        const PackOptions& options
    ) = 0;
};

} // namespace nxdev::pack
