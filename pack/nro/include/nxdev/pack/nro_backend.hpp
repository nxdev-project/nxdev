#pragma once

#include <nxdev/pack/backend.hpp>

namespace nxdev::pack {

class NroPackBackend : public IPackBackend {
public:
    NroPackBackend() = default;
    ~NroPackBackend() override = default;

    [[nodiscard]] PackageFormat get_format() const noexcept override {
        return PackageFormat::NRO;
    }

    [[nodiscard]] std::string get_format_name() const noexcept override {
        return "NRO";
    }

    [[nodiscard]] PackOperationResult pack(
        const manifest::Manifest& manifest,
        const PackOptions& options
    ) override;
};

} // namespace nxdev::pack
