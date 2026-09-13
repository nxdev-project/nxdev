#include "test_common.hpp"
#include <nxdev/manifest/parser.hpp>
#include <nxdev/manifest/diagnostics.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string resolve_test_path(const std::string& rel) {
#ifdef NXDEV_SOURCE_DIR
    fs::path base = NXDEV_SOURCE_DIR;
    return (base / rel).lexically_normal().string();
#else
    return rel;
#endif
}

int main() {
    std::cout << "[Test] Running NXDevAppManifest Test Suite...\n";

    nxdev::manifest::Parser parser;

    // 1. Test Minimal Valid Manifest
    {
        std::string minimal_yaml = R"(
schemaVersion: 1
application:
  name: "MinimalApp"
  author: "TestAuthor"
)";
        auto res = parser.parse_string(minimal_yaml);
        NXDEV_TEST_ASSERT(res.has_value());
        const auto& manifest = res.value();
        NXDEV_TEST_ASSERT(manifest.schema_version() == 1);
        NXDEV_TEST_ASSERT(manifest.application().name == "MinimalApp");
        NXDEV_TEST_ASSERT(manifest.application().author == "TestAuthor");
        NXDEV_TEST_ASSERT(manifest.application().version == "1.0.0"); // default
        NXDEV_TEST_ASSERT(manifest.assets().icon.type == nxdev::manifest::IconSourceType::LibnxDefault);
        NXDEV_TEST_ASSERT(!manifest.assets().romfs.enabled);
        NXDEV_TEST_ASSERT(manifest.packaging().default_format == nxdev::manifest::PackageFormat::NRO);
        NXDEV_TEST_ASSERT(manifest.nacp().display_version == "1.0.0");
        std::cout << "  ✓ Minimal manifest passed\n";
    }

    // 2. Test Example Manifest Files
    {
        auto min_res = parser.parse_file(resolve_test_path("examples/manifests/minimal/nxapp.yaml"));
        NXDEV_TEST_ASSERT(min_res.has_value());

        auto std_res = parser.parse_file(resolve_test_path("examples/manifests/standard/nxapp.yaml"));
        NXDEV_TEST_ASSERT(std_res.has_value());
        NXDEV_TEST_ASSERT(std_res.value().assets().icon.type == nxdev::manifest::IconSourceType::ProjectFile);
        NXDEV_TEST_ASSERT(std_res.value().assets().romfs.enabled);

        auto adv_res = parser.parse_file(resolve_test_path("examples/manifests/advanced/nxapp.yaml"));
        NXDEV_TEST_ASSERT(adv_res.has_value());
        NXDEV_TEST_ASSERT(adv_res.value().application().title_id.has_value());
        NXDEV_TEST_ASSERT(adv_res.value().application().title_id_numeric.has_value());
        NXDEV_TEST_ASSERT(adv_res.value().application().localized.size() == 3);
        std::cout << "  ✓ Example files passed validation\n";
    }

    // 3. Test Unsupported Schema Version
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/invalid_schema_version.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        NXDEV_TEST_ASSERT(res.diagnostics.has_errors());
        bool found_code = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::UnsupportedSchemaVersion) found_code = true;
        }
        NXDEV_TEST_ASSERT(found_code);
        std::cout << "  ✓ Unsupported schemaVersion correctly rejected\n";
    }

    // 4. Test Missing Application Section
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/missing_application.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        NXDEV_TEST_ASSERT(res.diagnostics.has_errors());
        std::cout << "  ✓ Missing application section correctly rejected\n";
    }

    // 5. Test Missing Required Fields
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/missing_required_fields.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        NXDEV_TEST_ASSERT(res.diagnostics.has_errors());
        std::cout << "  ✓ Missing required fields correctly rejected\n";
    }

    // 6. Test Malformed Title ID
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/malformed_title_id.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_tid_err = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::InvalidTitleId) found_tid_err = true;
        }
        NXDEV_TEST_ASSERT(found_tid_err);
        std::cout << "  ✓ Malformed titleId correctly rejected\n";
    }

    // 7. Test Unknown Fields (Strict Validation)
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/unknown_fields.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_unknown = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::UnknownField) found_unknown = true;
        }
        NXDEV_TEST_ASSERT(found_unknown);
        std::cout << "  ✓ Unknown fields correctly rejected\n";
    }

    // 8. Test Duplicate Dependencies
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/duplicate_dependencies.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_dup = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::DuplicateDependency) found_dup = true;
        }
        NXDEV_TEST_ASSERT(found_dup);
        std::cout << "  ✓ Duplicate dependencies correctly rejected\n";
    }

    // 9. Test Invalid Icon (Non-JPEG)
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/invalid_icon_not_jpeg.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_icon_err = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::InvalidAssetFormat) found_icon_err = true;
        }
        NXDEV_TEST_ASSERT(found_icon_err);
        std::cout << "  ✓ Non-JPEG icon extension correctly rejected\n";
    }

    // 10. Test Missing Icon File
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/missing_icon_file.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_missing = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::AssetNotFound) found_missing = true;
        }
        NXDEV_TEST_ASSERT(found_missing);
        std::cout << "  ✓ Missing icon file correctly rejected\n";
    }

    // 11. Test Invalid RomFS Path (File instead of directory)
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/invalid_romfs_file.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_romfs_err = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::InvalidRomFsPath) found_romfs_err = true;
        }
        NXDEV_TEST_ASSERT(found_romfs_err);
        std::cout << "  ✓ Invalid RomFS path correctly rejected\n";
    }

    // 12. Test Invalid Locale
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/invalid_locale.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_loc_err = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::InvalidLocaleCode) found_loc_err = true;
        }
        NXDEV_TEST_ASSERT(found_loc_err);
        std::cout << "  ✓ Invalid locale code correctly rejected\n";
    }

    // 13. Test Malformed YAML Syntax
    {
        auto res = parser.parse_file(resolve_test_path("tests/fixtures/malformed_yaml.yaml"));
        NXDEV_TEST_ASSERT(!res.has_value());
        bool found_syntax = false;
        for (const auto& d : res.diagnostics.diagnostics()) {
            if (d.code == nxdev::manifest::DiagnosticCode::YamlSyntaxError) found_syntax = true;
        }
        NXDEV_TEST_ASSERT(found_syntax);
        std::cout << "  ✓ Malformed YAML syntax correctly rejected\n";
    }

    // 14. Test Discovery Mechanism
    {
        auto discovered = parser.discover_manifest(resolve_test_path("examples/manifests/standard/assets"));
        NXDEV_TEST_ASSERT(discovered.has_value());
        NXDEV_TEST_ASSERT(discovered->find("examples/manifests/standard/nxapp.yaml") != std::string::npos);

        auto disc_load = parser.load_from_discovery(resolve_test_path("examples/manifests/standard/romfs"));
        NXDEV_TEST_ASSERT(disc_load.has_value());
        NXDEV_TEST_ASSERT(disc_load.value().application().name == "Standard Homebrew Game");
        std::cout << "  ✓ Discovery from subdirectories passed\n";
    }

    // 15. Test Serialization
    {
        auto res = parser.parse_file(resolve_test_path("examples/manifests/advanced/nxapp.yaml"));
        NXDEV_TEST_ASSERT(res.has_value());
        std::string json = res.value().to_json();
        NXDEV_TEST_ASSERT(json.find("\"name\": \"Advanced NXDev Engine\"") != std::string::npos);
        NXDEV_TEST_ASSERT(json.find("\"schemaVersion\": 1") != std::string::npos);

        std::string human = res.value().to_human_readable();
        NXDEV_TEST_ASSERT(human.find("NXDevAppManifest (schemaVersion: 1)") != std::string::npos);
        std::cout << "  ✓ Human-readable and JSON serialization passed\n";
    }

    std::cout << "[Test] All NXDevAppManifest tests passed successfully!\n";
    return 0;
}
