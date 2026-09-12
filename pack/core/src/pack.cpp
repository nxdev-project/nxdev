#include <nxdev/pack/pack.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/nsp_backend.hpp>

namespace nxdev::pack {

PackManager::PackManager() {
    register_backend(std::make_unique<NroPackBackend>());
    register_backend(std::make_unique<NspPackBackend>());
}

void PackManager::register_backend(std::unique_ptr<IPackBackend> backend) {
    if (backend) {
        backends_[backend->get_format()] = std::move(backend);
    }
}

IPackBackend* PackManager::get_backend(PackageFormat format) const noexcept {
    auto it = backends_.find(format);
    if (it != backends_.end()) {
        return it->second.get();
    }
    return nullptr;
}

PackOperationResult PackManager::pack(
    const manifest::Manifest& manifest,
    const PackOptions& options
) {
    auto* backend = get_backend(options.format);
    if (!backend) {
        return PackOperationResult{
            .success = false,
            .result = {},
            .error = {
                .code = PackErrorCode::ToolNotFound,
                .message = "No packaging backend registered for requested format"
            }
        };
    }

    return backend->pack(manifest, options);
}

} // namespace nxdev::pack
