#include "test_common.hpp"
#include <nxdev/packages/registry.hpp>
#include <nxdev/packages/resolver.hpp>
#include <nxdev/packages/package_manager.hpp>
#include <nxdev/env/environment.hpp>

using namespace nxdev::packages;

void test_registry_default() {
    auto reg = PackageRegistry::create_default();
    NXDEV_TEST_ASSERT(!reg.empty());
    NXDEV_TEST_ASSERT(reg.size() >= 15);

    auto val_res = reg.validate();
    NXDEV_TEST_ASSERT(val_res.is_success());

    const auto* core = reg.find("nxdev.core");
    NXDEV_TEST_ASSERT(core != nullptr);
    NXDEV_TEST_ASSERT(core->is_builtin());

    const auto* sdl2 = reg.find("nxdev.sdl2");
    NXDEV_TEST_ASSERT(sdl2 != nullptr);
    NXDEV_TEST_ASSERT(sdl2->is_devkitpro());
    NXDEV_TEST_ASSERT(!sdl2->devkitpro.packages.empty());
    NXDEV_TEST_ASSERT(sdl2->devkitpro.packages[0] == "switch-sdl2");

    // Test alias lookup
    const auto* sdl2_alias = reg.find("sdl2");
    NXDEV_TEST_ASSERT(sdl2_alias == sdl2);

    const auto* dkp_alias = reg.find("switch-sdl2");
    NXDEV_TEST_ASSERT(dkp_alias == sdl2);
}

void test_registry_json_parsing() {
    std::string json = R"({
        "schema_version": "1.0",
        "packages": [
            {
                "id": "nxdev.core",
                "name": "Core",
                "description": "Core SDK",
                "kind": "builtin",
                "category": "core",
                "dependencies": []
            },
            {
                "id": "nxdev.testpkg",
                "name": "Test Package",
                "description": "Test portlib",
                "kind": "devkitpro",
                "category": "graphics",
                "dependencies": ["nxdev.core"],
                "devkitpro": {
                    "packages": ["switch-testpkg"]
                },
                "cmake": {
                    "targets": ["NXDev::TestPkg"]
                }
            }
        ]
    })";

    auto reg_res = PackageRegistry::load_from_string(json);
    if (!reg_res.is_success()) {
        std::cerr << "load_from_string failed with error: " << reg_res.error_name() << "\n";
    }
    NXDEV_TEST_ASSERT(reg_res.is_success());
    const auto& reg = reg_res.value();
    NXDEV_TEST_ASSERT(reg.size() == 2);

    const auto* testpkg = reg.find("nxdev.testpkg");
    NXDEV_TEST_ASSERT(testpkg != nullptr);
    NXDEV_TEST_ASSERT(testpkg->cmake.targets[0] == "NXDev::TestPkg");
}

void test_registry_validation_errors() {
    // 1. Invalid schema version
    {
        std::string json = R"({"schema_version": "99.0", "packages": []})";
        auto res = PackageRegistry::load_from_string(json);
        NXDEV_TEST_ASSERT(!res.is_success());
    }

    // 2. Duplicate ID
    {
        std::string json = R"({
            "schema_version": "1.0",
            "packages": [
                {"id": "nxdev.a", "name": "A", "description": "A", "kind": "builtin", "dependencies": []},
                {"id": "nxdev.a", "name": "A2", "description": "A2", "kind": "builtin", "dependencies": []}
            ]
        })";
        auto res = PackageRegistry::load_from_string(json);
        NXDEV_TEST_ASSERT(!res.is_success());
    }

    // 3. Unknown dependency
    {
        std::string json = R"({
            "schema_version": "1.0",
            "packages": [
                {"id": "nxdev.a", "name": "A", "description": "A", "kind": "builtin", "dependencies": ["nxdev.nonexistent"]}
            ]
        })";
        auto res = PackageRegistry::load_from_string(json);
        NXDEV_TEST_ASSERT(!res.is_success());
    }

    // 4. Cycle detection in registry
    {
        std::string json = R"({
            "schema_version": "1.0",
            "packages": [
                {"id": "nxdev.a", "name": "A", "description": "A", "kind": "builtin", "dependencies": ["nxdev.b"]},
                {"id": "nxdev.b", "name": "B", "description": "B", "kind": "builtin", "dependencies": ["nxdev.a"]}
            ]
        })";
        auto res = PackageRegistry::load_from_string(json);
        NXDEV_TEST_ASSERT(!res.is_success());
    }
}

void test_registry_search() {
    auto reg = PackageRegistry::create_default();
    
    auto results_sdl = reg.search("sdl");
    NXDEV_TEST_ASSERT(results_sdl.size() >= 4);

    auto results_audio = reg.search("audio");
    NXDEV_TEST_ASSERT(!results_audio.empty());

    auto results_curl = reg.search("curl");
    NXDEV_TEST_ASSERT(!results_curl.empty());
}

void test_dependency_resolver() {
    auto reg = PackageRegistry::create_default();
    DependencyResolver resolver(reg);

    // 1. Single dependency
    {
        auto res = resolver.resolve({"nxdev.core"});
        NXDEV_TEST_ASSERT(res.success);
        NXDEV_TEST_ASSERT(res.ordered_module_ids.size() == 1);
        NXDEV_TEST_ASSERT(res.ordered_module_ids[0] == "nxdev.core");
        NXDEV_TEST_ASSERT(res.builtin_modules.size() == 1);
        NXDEV_TEST_ASSERT(res.devkitpro_packages.empty());
    }

    // 2. Transitive dependency
    {
        auto res = resolver.resolve({"nxdev.sdl2-image"});
        NXDEV_TEST_ASSERT(res.success);
        NXDEV_TEST_ASSERT(res.ordered_module_ids.size() == 3);
        NXDEV_TEST_ASSERT(res.ordered_module_ids[0] == "nxdev.core");
        NXDEV_TEST_ASSERT(res.ordered_module_ids[1] == "nxdev.sdl2");
        NXDEV_TEST_ASSERT(res.ordered_module_ids[2] == "nxdev.sdl2-image");
        NXDEV_TEST_ASSERT(res.devkitpro_packages.size() == 2);
    }

    // 3. Deduplication of packages
    {
        auto res = resolver.resolve({"nxdev.sdl2", "nxdev.sdl2-image", "nxdev.sdl2-mixer"});
        NXDEV_TEST_ASSERT(res.success);
        // switch-sdl2 should appear only once in devkitpro_packages
        int sdl2_count = 0;
        for (const auto& pkg : res.devkitpro_packages) {
            if (pkg == "switch-sdl2") sdl2_count++;
        }
        NXDEV_TEST_ASSERT(sdl2_count == 1);
    }

    // 4. Unknown module
    {
        auto res = resolver.resolve({"nxdev.unknown_module"});
        NXDEV_TEST_ASSERT(!res.success);
        NXDEV_TEST_ASSERT(!res.errors.empty());
    }
}

void test_mock_package_manager() {
    auto reg = PackageRegistry::create_default();
    nxdev::config::HostConfig cfg;
    auto env = nxdev::env::Environment::detect(cfg);

    auto mock_backend = std::make_unique<MockPackageManagerBackend>();
    auto* mock_ptr = mock_backend.get();

    mock_ptr->set_package_installed("switch-sdl2", "2.28.5-4");
    mock_ptr->set_package_missing("switch-sdl2_image");

    PackageManager pm(env, reg, std::move(mock_backend));

    // Status checks
    const auto* sdl2 = reg.find("nxdev.sdl2");
    const auto* sdl2_img = reg.find("nxdev.sdl2-image");
    const auto* core = reg.find("nxdev.core");

    auto st_core = pm.check_module_status(*core);
    NXDEV_TEST_ASSERT(st_core.status == PackageStatus::Installed);

    auto st_sdl2 = pm.check_module_status(*sdl2);
    NXDEV_TEST_ASSERT(st_sdl2.status == PackageStatus::Installed);
    NXDEV_TEST_ASSERT(st_sdl2.detected_version == "2.28.5-4");

    auto st_img = pm.check_module_status(*sdl2_img);
    NXDEV_TEST_ASSERT(st_img.status == PackageStatus::Missing);
    NXDEV_TEST_ASSERT(!st_img.missing_system_packages.empty());

    // Dry-run install
    auto dry_res = pm.install({"nxdev.sdl2-image"}, true);
    NXDEV_TEST_ASSERT(dry_res.success);
    NXDEV_TEST_ASSERT(dry_res.dry_run);
    // Package should still be missing after dry run
    NXDEV_TEST_ASSERT(pm.check_module_status(*sdl2_img).status == PackageStatus::Missing);

    // Real install (via mock)
    auto install_res = pm.install({"nxdev.sdl2-image"}, false);
    NXDEV_TEST_ASSERT(install_res.success);
    NXDEV_TEST_ASSERT(pm.check_module_status(*sdl2_img).status == PackageStatus::Installed);

    // Remove
    auto remove_res = pm.remove({"nxdev.sdl2-image"}, false);
    NXDEV_TEST_ASSERT(remove_res.success);
    NXDEV_TEST_ASSERT(pm.check_module_status(*sdl2_img).status == PackageStatus::Missing);

    // Test failure simulation
    mock_ptr->set_operation_should_fail(true, 42, "Database locked by another process");
    auto fail_res = pm.install({"nxdev.sdl2-image"}, false);
    NXDEV_TEST_ASSERT(!fail_res.success);
    NXDEV_TEST_ASSERT(fail_res.exit_code == 42);
}

int main() {
    std::cout << "[TEST] Running Package Subsystem Tests...\n";

    test_registry_default();
    std::cout << "  ✓ test_registry_default\n";

    test_registry_json_parsing();
    std::cout << "  ✓ test_registry_json_parsing\n";

    test_registry_validation_errors();
    std::cout << "  ✓ test_registry_validation_errors\n";

    test_registry_search();
    std::cout << "  ✓ test_registry_search\n";

    test_dependency_resolver();
    std::cout << "  ✓ test_dependency_resolver\n";

    test_mock_package_manager();
    std::cout << "  ✓ test_mock_package_manager\n";

    std::cout << "All Package Subsystem Tests passed successfully!\n";
    return 0;
}
