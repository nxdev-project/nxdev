#pragma once

#include <nxdev/pack/backend.hpp>
#include <map>
#include <memory>

namespace nxdev::pack {

class PackManager {
public:
    PackManager();
    ~PackManager() = default;

    void register_backend(std::unique_ptr<IPackBackend> backend);
    [[nodiscard]] IPackBackend* get_backend(PackageFormat format) const noexcept;

    [[nodiscard]] PackageResult pack(
        const PackageRequest& request,
        const env::Environment* env = nullptr,
        ProgressCallback progress = nullptr
    );

    // Legacy method overload for compatibility with older call signatures
    [[nodiscard]] PackOperationResult pack(
        const manifest::Manifest& manifest,
        const PackOptions& options
    );

private:
    std::map<PackageFormat, std::unique_ptr<IPackBackend>> backends_;
};

} // namespace nxdev::pack
