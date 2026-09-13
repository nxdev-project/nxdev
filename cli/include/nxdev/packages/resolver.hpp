#pragma once

#include <nxdev/packages/registry.hpp>
#include <nxdev/manifest/manifest.hpp>
#include <string>
#include <vector>

namespace nxdev::packages {

struct ResolutionResult {
    bool success{true};
    std::vector<std::string> requested_ids;
    std::vector<std::string> ordered_module_ids;       // Topological order (dependencies first)
    std::vector<PackageDefinition> resolved_packages;  // Corresponding package definitions in order
    std::vector<std::string> builtin_modules;          // Filtered list of built-in module IDs
    std::vector<std::string> devkitpro_modules;        // Filtered list of devkitPro logical module IDs
    std::vector<std::string> devkitpro_packages;       // Deduplicated list of underlying devkitPro pacman packages
    std::vector<std::string> cmake_targets;            // Deduplicated CMake interface targets
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] std::string to_json() const;
};

class DependencyResolver {
public:
    explicit DependencyResolver(const PackageRegistry& registry);
    ~DependencyResolver() = default;

    /**
     * @brief Resolves a list of requested NXDev module IDs (or aliases), performing transitive closure and topological sort.
     */
    [[nodiscard]] ResolutionResult resolve(const std::vector<std::string>& requested_ids) const;

    /**
     * @brief Resolves all dependencies declared in an NXDevAppManifest.
     */
    [[nodiscard]] ResolutionResult resolve_manifest(const manifest::Manifest& manifest) const;

private:
    const PackageRegistry& registry_;
};

} // namespace nxdev::packages
