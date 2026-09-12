#pragma once

#include <nxdev/pack/backend.hpp>

namespace nxdev::pack {

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

    [[nodiscard]] PackOperationResult pack(
        const manifest::Manifest& manifest,
        const PackOptions& options
    ) override;
};

} // namespace nxdev::pack
