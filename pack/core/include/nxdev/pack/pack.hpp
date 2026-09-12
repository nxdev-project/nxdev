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

    [[nodiscard]] PackOperationResult pack(
        const manifest::Manifest& manifest,
        const PackOptions& options
    );

private:
    std::map<PackageFormat, std::unique_ptr<IPackBackend>> backends_;
};

} // namespace nxdev::pack
