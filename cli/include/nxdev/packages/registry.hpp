#pragma once

#include <nxdev/packages/package_definition.hpp>
#include <nxdev/result.hpp>
#include <nxdev/env/environment.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace nxdev::packages {

class PackageRegistry {
public:
    PackageRegistry() = default;
    ~PackageRegistry() = default;

    /**
     * @brief Loads registry definitions from a JSON or YAML file.
     */
    static Result<PackageRegistry> load_from_file(const std::string& filepath);

    /**
     * @brief Loads registry definitions from a JSON or YAML string.
     */
    static Result<PackageRegistry> load_from_string(const std::string& content);

    /**
     * @brief Loads default bundled package registry using environment paths with built-in fallback.
     */
    static Result<PackageRegistry> load_default(const env::Environment& env);

    /**
     * @brief Creates a default in-memory registry containing all standard built-in and devkitPro packages.
     */
    static PackageRegistry create_default();

    [[nodiscard]] const std::string& schema_version() const noexcept { return schema_version_; }
    void set_schema_version(std::string ver) { schema_version_ = std::move(ver); }

    [[nodiscard]] const std::vector<PackageDefinition>& packages() const noexcept { return packages_; }
    [[nodiscard]] size_t size() const noexcept { return packages_.size(); }
    [[nodiscard]] bool empty() const noexcept { return packages_.empty(); }

    /**
     * @brief Finds a package by canonical ID (e.g. "nxdev.sdl2") or alias (e.g. "sdl2").
     */
    [[nodiscard]] const PackageDefinition* find(const std::string& id_or_alias) const;

    /**
     * @brief Checks whether a package or alias exists in the registry.
     */
    [[nodiscard]] bool contains(const std::string& id_or_alias) const;

    /**
     * @brief Searches for packages matching query string across ID, name, description, category, and devkitpro packages.
     */
    [[nodiscard]] std::vector<const PackageDefinition*> search(const std::string& query) const;

    /**
     * @brief Adds or replaces a package definition.
     */
    void add_package(PackageDefinition pkg);

    /**
     * @brief Validates entire registry graph (schema version, duplicate IDs, missing deps, cycles).
     */
    [[nodiscard]] Result<void> validate() const;

    /**
     * @brief Serializes the complete registry to formatted JSON.
     */
    [[nodiscard]] std::string to_json() const;

private:
    std::string schema_version_{"1.0"};
    std::vector<PackageDefinition> packages_;
    std::unordered_map<std::string, size_t> index_by_id_;
    std::unordered_map<std::string, std::string> aliases_;

    void rebuild_indices();
};

} // namespace nxdev::packages
