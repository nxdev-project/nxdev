#include <nxdev/pack/nsp_backend.hpp>

namespace nxdev::pack {

PackOperationResult NspPackBackend::pack(
    [[maybe_unused]] const manifest::Manifest& manifest,
    [[maybe_unused]] const PackOptions& options
) {
    return PackOperationResult{
        .success = false,
        .result = {},
        .error = {
            .code = PackErrorCode::NotImplemented,
            .message = "NSP packaging backend is in development and not yet implemented."
        }
    };
}

} // namespace nxdev::pack
