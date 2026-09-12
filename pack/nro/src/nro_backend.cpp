#include <nxdev/pack/nro_backend.hpp>

namespace nxdev::pack {

PackOperationResult NroPackBackend::pack(
    [[maybe_unused]] const manifest::Manifest& manifest,
    [[maybe_unused]] const PackOptions& options
) {
    return PackOperationResult{
        .success = false,
        .result = {},
        .error = {
            .code = PackErrorCode::NotImplemented,
            .message = "NRO packaging backend is in development and not yet implemented."
        }
    };
}

} // namespace nxdev::pack
