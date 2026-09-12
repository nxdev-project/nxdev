#include "test_common.hpp"
#include <nxdev/pack/pack.hpp>
#include <iostream>

int main() {
    std::cout << "[Test] Running Pack Manager tests...\n";

    nxdev::pack::PackManager manager;

    // Verify backends are registered
    auto* nro_backend = manager.get_backend(nxdev::pack::PackageFormat::NRO);
    NXDEV_TEST_ASSERT(nro_backend != nullptr);
    NXDEV_TEST_ASSERT(nro_backend->get_format() == nxdev::pack::PackageFormat::NRO);
    NXDEV_TEST_ASSERT(nro_backend->get_format_name() == "NRO");

    auto* nsp_backend = manager.get_backend(nxdev::pack::PackageFormat::NSP);
    NXDEV_TEST_ASSERT(nsp_backend != nullptr);
    NXDEV_TEST_ASSERT(nsp_backend->get_format() == nxdev::pack::PackageFormat::NSP);
    NXDEV_TEST_ASSERT(nsp_backend->get_format_name() == "NSP");

    // Packing at this skeleton stage returns NotImplemented
    nxdev::manifest::Manifest dummy_manifest;
    dummy_manifest.app.name = "TestApp";

    nxdev::pack::PackOptions nro_opts{};
    nro_opts.format = nxdev::pack::PackageFormat::NRO;
    nro_opts.input_elf_path = "app.elf";
    nro_opts.output_path = "app.nro";

    auto nro_res = manager.pack(dummy_manifest, nro_opts);
    NXDEV_TEST_ASSERT(!nro_res.has_value());
    NXDEV_TEST_ASSERT(nro_res.error_info().code == nxdev::pack::PackErrorCode::NotImplemented);

    nxdev::pack::PackOptions nsp_opts{};
    nsp_opts.format = nxdev::pack::PackageFormat::NSP;
    nsp_opts.input_elf_path = "app.elf";
    nsp_opts.output_path = "app.nsp";

    auto nsp_res = manager.pack(dummy_manifest, nsp_opts);
    NXDEV_TEST_ASSERT(!nsp_res.has_value());
    NXDEV_TEST_ASSERT(nsp_res.error_info().code == nxdev::pack::PackErrorCode::NotImplemented);

    std::cout << "[Test] Pack Manager tests passed successfully.\n";
    return 0;
}
