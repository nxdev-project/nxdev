#pragma once

#include <nxdev/manifest/manifest.hpp>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <functional>

namespace nxdev::pack {

/**
 * @brief Represents a single resource layer contributing to the staged RomFS.
 */
struct RomFsLayer {
    std::string name;             ///< Identifying name (e.g. "borealis", "project")
    std::string source_path;      ///< Absolute or relative path to host directory
    std::string target_prefix;    ///< Subpath prefix in staged RomFS (e.g. "resources", "" for root)
    int priority{100};            ///< Copy order priority (e.g. 100 for framework, 1000 for user). Higher priority copies last (wins).
};

/**
 * @brief Request parameters for RomFS staging.
 */
struct RomFsStageRequest {
    std::string project_root;
    std::string output_dir;                      ///< Destination directory (e.g. .nxdev/build/<profile>/romfs)
    std::optional<std::string> user_romfs_path;  ///< Explicit or manifest user RomFS directory
    std::vector<RomFsLayer> framework_layers;    ///< Framework layers to stage before user RomFS
    bool clean{true};                            ///< Safely clear output directory before staging
    bool verbose{false};
    bool strict_collision{false};                ///< Fail on collisions instead of user-wins overlay
    std::string sdk_root;                        ///< SDK root for locating installed framework assets
};

/**
 * @brief Detailed entry recorded for each file in the staging manifest.
 */
struct RomFsManifestEntry {
    std::string relative_path;   ///< Destination path relative to staged RomFS root
    std::string source_layer;    ///< Layer name that provided the final file
    std::string source_file;     ///< Absolute source path on host
    std::string sha256;          ///< SHA-256 hash of final file contents
    size_t size_bytes{0};        ///< File size in bytes
    std::string overridden_layer;///< Layer name overridden by this file (empty if no collision)
};

/**
 * @brief Result returned from RomFsStager::stage.
 */
struct RomFsStageResult {
    bool success{false};
    bool has_romfs{false};                       ///< True if any resources were staged
    std::string staged_dir;                      ///< Path to the final staged RomFS directory
    std::string manifest_path;                   ///< Path to the generated romfs-manifest.json
    size_t files_copied{0};                      ///< Total files in final staged tree
    size_t overridden_files{0};                  ///< Number of framework files overridden by user
    std::vector<std::string> source_layers;      ///< Names of layers included
    std::string fingerprint;                     ///< Combined cryptographic fingerprint of RomFS contents
    std::vector<RomFsManifestEntry> entries;     ///< Detailed manifest entries
    std::vector<std::string> warnings;           ///< Non-fatal diagnostics
    std::string error_message;                   ///< Error description if success is false

    [[nodiscard]] std::string to_json() const;
};

/**
 * @brief Shared Host-Side RomFS Staging Pipeline.
 *
 * Ensures RomFS assets are safely validated, layered, and staged into an NXDev-owned
 * build directory prior to NRO/NSP packaging, guaranteeing the user's source directory
 * is never modified and framework assets (like Borealis) are seamlessly overlaid.
 */
class RomFsStager {
public:
    /**
     * @brief Execute the full RomFS staging pipeline.
     */
    [[nodiscard]] static RomFsStageResult stage(const RomFsStageRequest& request);

    /**
     * @brief Resolve standard RomFS layers for a project manifest.
     * Automatically adds Borealis framework resources if nxdev.borealis dependency is present.
     */
    [[nodiscard]] static std::vector<RomFsLayer> resolve_manifest_layers(
        const manifest::Manifest& manifest,
        const std::string& project_root,
        const std::string& sdk_root = ""
    );

    /**
     * @brief Locate Borealis framework resources directory on the host.
     */
    [[nodiscard]] static std::string locate_borealis_resources(
        const std::string& project_root,
        const std::string& sdk_root = ""
    );

    /**
     * @brief Validate that essential Borealis framework assets exist in the staged directory.
     */
    [[nodiscard]] static bool validate_essential_borealis_resources(
        const std::string& staged_dir,
        std::string& error_msg
    );

    /**
     * @brief Check whether a directory path is safe from recursion / overlap with staging.
     */
    [[nodiscard]] static bool is_safe_staging_path(
        const std::string& candidate_path,
        const std::string& project_root,
        std::string& error_msg
    );

    /**
     * @brief Compute SHA-256 for a given host file.
     */
    [[nodiscard]] static std::string compute_file_sha256(const std::string& file_path);
};

} // namespace nxdev::pack
