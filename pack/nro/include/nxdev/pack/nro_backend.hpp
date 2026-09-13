#pragma once

#include <nxdev/pack/backend.hpp>
#include <filesystem>

namespace nxdev::pack {

class NroPackBackend : public IPackBackend {
public:
    NroPackBackend() = default;
    ~NroPackBackend() override = default;

    [[nodiscard]] PackageFormat get_format() const noexcept override {
        return PackageFormat::NRO;
    }

    [[nodiscard]] std::string get_format_name() const noexcept override {
        return "NRO";
    }

    [[nodiscard]] PackageResult pack(
        const PackageRequest& request,
        const env::Environment* env = nullptr,
        ProgressCallback progress = nullptr
    ) override;

    /**
     * @brief Validates an ELF binary to ensure it exists, is readable, and has AArch64 Switch ELF headers.
     */
    static bool validate_elf_binary(const std::string& elf_path, std::string& error_message);

    /**
     * @brief Checks if a file is a valid JPEG image (magic 0xFF 0xD8 0xFF).
     */
    static bool is_valid_jpeg(const std::string& image_path, std::string& error_message);

    /**
     * @brief Validates an NRO binary to ensure it has valid size and NRO0 magic at offset 0x10.
     */
    static bool validate_nro_binary(const std::string& nro_path, std::string& error_message);

    /**
     * @brief Sanitizes an application name for use as a safe, portable file basename.
     */
    static std::string sanitize_filename(std::string_view name);
};

} // namespace nxdev::pack
