#pragma once

#include <nxdev/pack/backend.hpp>
#include <string>
#include <vector>
#include <optional>

namespace nxdev::pack {

class HacBrewPackAdapter {
public:
    static constexpr const char* DEFAULT_BACKEND_NAME = "hacBrewPack";
    static constexpr const char* PINNED_BACKEND_VERSION = "3.05";

    /**
     * @brief Generates JSON content compatible with npdmtool based on manifest NPDM configuration.
     */
    static bool generate_npdm_json(
        const manifest::Manifest& manifest,
        std::string& out_json,
        std::string& error_msg
    );

    /**
     * @brief Resolves key file in order of precedence:
     * 1. Explicit request override / CLI flag
     * 2. NXDEV_KEYS environment variable
     * 3. Project-local or global config
     * 4. Standard Switch default locations (~/.switch/prod.keys, ~/.switch/keys.dat)
     */
    static std::string resolve_keys_file(
        const PackageRequest& request,
        const env::Environment* env = nullptr
    );

    /**
     * @brief Validates that a file has a valid PFS0 (NSP) header.
     */
    static bool validate_pfs0_binary(
        const std::string& nsp_path,
        std::string& error_msg
    );

    /**
     * @brief Sanitizes application name for safe file naming.
     */
    static std::string sanitize_filename(std::string_view name);
};

class NspPackBackend : public IPackBackend {
public:
    NspPackBackend() = default;
    ~NspPackBackend() override = default;

    [[nodiscard]] PackageFormat get_format() const noexcept override {
        return PackageFormat::NSP;
    }

    [[nodiscard]] std::string get_format_name() const noexcept override {
        return "NSP";
    }

    [[nodiscard]] PackageResult pack(
        const PackageRequest& request,
        const env::Environment* env = nullptr,
        ProgressCallback progress = nullptr
    ) override;
};

} // namespace nxdev::pack
