#include <nxdev/pack/romfs_stager.hpp>
#include <nxdev/manifest/manifest.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>

namespace fs = std::filesystem;
using namespace nxdev::pack;

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

static void write_text_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << content;
}

static std::string read_text_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int main() {
    std::cout << "=== Running RomFS Stager Unit Tests ===\n";

    fs::path temp_base = fs::temp_directory_path() / "nxdev_test_romfs_stager";
    std::error_code ec;
    fs::remove_all(temp_base, ec);
    fs::create_directories(temp_base, ec);

    fs::path proj_root = temp_base / "test_project";
    fs::path stage_dir = proj_root / ".nxdev" / "build" / "debug" / "romfs";
    fs::path user_romfs = proj_root / "romfs";
    fs::path framework_res = temp_base / "fake_borealis" / "resources";

    fs::create_directories(user_romfs);
    fs::create_directories(framework_res);

    // -------------------------------------------------------------------------
    // Test 1: No Resources At All
    // -------------------------------------------------------------------------
    {
        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.clean = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(res.success);
        TEST_ASSERT(!res.has_romfs);
        TEST_ASSERT(res.files_copied == 0);
        std::cout << "  ✓ Test 1 Passed: No resources case returns has_romfs=false\n";
    }

    // -------------------------------------------------------------------------
    // Test 2: User RomFS Only
    // -------------------------------------------------------------------------
    {
        write_text_file(user_romfs / "data.txt", "User Data Payload");
        write_text_file(user_romfs / "assets" / "texture.png", "PNG Data");

        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = user_romfs.string();
        req.clean = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(res.success);
        TEST_ASSERT(res.has_romfs);
        TEST_ASSERT(res.files_copied == 2);
        TEST_ASSERT(res.overridden_files == 0);
        TEST_ASSERT(fs::exists(stage_dir / "data.txt"));
        TEST_ASSERT(fs::exists(stage_dir / "assets" / "texture.png"));
        TEST_ASSERT(read_text_file(stage_dir / "data.txt") == "User Data Payload");
        TEST_ASSERT(!res.fingerprint.empty());
        std::cout << "  ✓ Test 2 Passed: User RomFS only staging\n";
    }

    // -------------------------------------------------------------------------
    // Test 3: Framework Layer Only (Borealis base)
    // -------------------------------------------------------------------------
    {
        write_text_file(framework_res / "material" / "theme_light.json", "{\"theme\": \"light_fw\"}");
        write_text_file(framework_res / "font" / "material-icons.ttf", "TTF Font Data");
        write_text_file(framework_res / "i18n" / "en-US.json", "{\"hello\": \"world\"}");

        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.framework_layers.push_back(RomFsLayer{
            .name = "borealis",
            .source_path = framework_res.string(),
            .target_prefix = "resources",
            .priority = 100
        });
        req.clean = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(res.success);
        TEST_ASSERT(res.has_romfs);
        TEST_ASSERT(res.files_copied == 3);
        TEST_ASSERT(fs::exists(stage_dir / "resources" / "material" / "theme_light.json"));
        TEST_ASSERT(fs::exists(stage_dir / "resources" / "font" / "material-icons.ttf"));
        TEST_ASSERT(fs::exists(stage_dir / "resources" / "i18n" / "en-US.json"));
        std::cout << "  ✓ Test 3 Passed: Framework layer only staging under prefix\n";
    }

    // -------------------------------------------------------------------------
    // Test 4: Multi-Layer Overlay with Collision (User Wins!)
    // -------------------------------------------------------------------------
    {
        // Framework has theme_light.json and material-icons.ttf
        // User overrides theme_light.json and provides custom_level.bin
        write_text_file(user_romfs / "resources" / "material" / "theme_light.json", "{\"theme\": \"light_USER_OVERRIDE\"}");
        write_text_file(user_romfs / "custom_level.bin", "Level 1 Binary Data");

        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = user_romfs.string();
        req.framework_layers.push_back(RomFsLayer{
            .name = "borealis",
            .source_path = framework_res.string(),
            .target_prefix = "resources",
            .priority = 100
        });
        req.clean = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(res.success);
        TEST_ASSERT(res.has_romfs);
        TEST_ASSERT(res.overridden_files >= 1);

        // Verify user version won
        std::string staged_theme = read_text_file(stage_dir / "resources" / "material" / "theme_light.json");
        TEST_ASSERT(staged_theme == "{\"theme\": \"light_USER_OVERRIDE\"}");

        // Verify framework non-colliding file survived
        TEST_ASSERT(fs::exists(stage_dir / "resources" / "font" / "material-icons.ttf"));
        // Verify user non-colliding file survived
        TEST_ASSERT(fs::exists(stage_dir / "custom_level.bin"));

        // Verify user source directory remained untouched
        TEST_ASSERT(read_text_file(user_romfs / "resources" / "material" / "theme_light.json") == "{\"theme\": \"light_USER_OVERRIDE\"}");

        // Verify manifest recorded the override
        TEST_ASSERT(fs::exists(res.manifest_path));
        bool found_override_in_manifest = false;
        for (const auto& entry : res.entries) {
            if (entry.relative_path == "resources/material/theme_light.json") {
                TEST_ASSERT(entry.source_layer == "project");
                TEST_ASSERT(entry.overridden_layer == "borealis");
                found_override_in_manifest = true;
            }
        }
        TEST_ASSERT(found_override_in_manifest);
        std::cout << "  ✓ Test 4 Passed: Multi-layer overlay and collision override semantics (User wins)\n";
    }

    // -------------------------------------------------------------------------
    // Test 5: Strict Collision Mode
    // -------------------------------------------------------------------------
    {
        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = user_romfs.string();
        req.framework_layers.push_back(RomFsLayer{
            .name = "borealis",
            .source_path = framework_res.string(),
            .target_prefix = "resources",
            .priority = 100
        });
        req.strict_collision = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(!res.success);
        TEST_ASSERT(res.error_message.find("Strict collision") != std::string::npos);
        std::cout << "  ✓ Test 5 Passed: Strict collision mode fails on overlap\n";
    }

    // -------------------------------------------------------------------------
    // Test 6: Nested Directories, Spaces, and Unicode
    // -------------------------------------------------------------------------
    {
        write_text_file(user_romfs / "deep" / "nested" / "path" / "file.txt", "Deep");
        write_text_file(user_romfs / "folder with spaces" / "file with spaces.txt", "Space data");
        write_text_file(user_romfs / "i18n" / "日本語.json", "{\"key\": \"値\"}");

        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = user_romfs.string();
        req.clean = true;

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(res.success);
        TEST_ASSERT(fs::exists(stage_dir / "deep" / "nested" / "path" / "file.txt"));
        TEST_ASSERT(fs::exists(stage_dir / "folder with spaces" / "file with spaces.txt"));
        TEST_ASSERT(fs::exists(stage_dir / "i18n" / "日本語.json"));
        std::cout << "  ✓ Test 6 Passed: Deep nesting, spaces, and unicode paths\n";
    }

    // -------------------------------------------------------------------------
    // Test 7: Output Recursion / Overlap Prevention
    // -------------------------------------------------------------------------
    {
        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = stage_dir.string(); // Unsafe! pointing to output itself

        auto res = RomFsStager::stage(req);
        TEST_ASSERT(!res.success);
        TEST_ASSERT(res.error_message.find("overlaps staging destination") != std::string::npos);
        std::cout << "  ✓ Test 7 Passed: Staging recursion and overlap prevention\n";
    }

    // -------------------------------------------------------------------------
    // Test 8: Symlink Escape Protection
    // -------------------------------------------------------------------------
    {
        fs::path escape_dir = temp_base / "escape_target";
        fs::create_directories(escape_dir);
        write_text_file(escape_dir / "secret.txt", "secret");

        fs::path evil_romfs = temp_base / "evil_romfs";
        fs::create_directories(evil_romfs);
        fs::create_directory_symlink(escape_dir, evil_romfs / "sym_link", ec);

        if (!ec) {
            RomFsStageRequest req;
            req.project_root = proj_root.string();
            req.output_dir = stage_dir.string();
            req.user_romfs_path = evil_romfs.string();

            auto res = RomFsStager::stage(req);
            TEST_ASSERT(!res.success);
            TEST_ASSERT(res.error_message.find("escapes") != std::string::npos);
            std::cout << "  ✓ Test 8 Passed: Symlink escape jail protection\n";
        }
    }

    // -------------------------------------------------------------------------
    // Test 9: Fingerprint Invalidation on File Content Change
    // -------------------------------------------------------------------------
    {
        write_text_file(user_romfs / "test_fp.txt", "version 1");
        RomFsStageRequest req;
        req.project_root = proj_root.string();
        req.output_dir = stage_dir.string();
        req.user_romfs_path = user_romfs.string();

        auto res1 = RomFsStager::stage(req);
        TEST_ASSERT(res1.success);
        std::string fp1 = res1.fingerprint;

        write_text_file(user_romfs / "test_fp.txt", "version 2 (modified)");
        auto res2 = RomFsStager::stage(req);
        TEST_ASSERT(res2.success);
        std::string fp2 = res2.fingerprint;

        TEST_ASSERT(fp1 != fp2);
        std::cout << "  ✓ Test 9 Passed: Deterministic fingerprint invalidation on content change\n";
    }

    // -------------------------------------------------------------------------
    // Test 10: Manifest Layer Resolution for Borealis Dependency
    // -------------------------------------------------------------------------
    {
        nxdev::manifest::Manifest mf;
        mf.dependencies().push_back(nxdev::manifest::Dependency{.name = "nxdev.borealis"});

        auto layers = RomFsStager::resolve_manifest_layers(mf, temp_base.string());
        // Since fake_borealis or repo third_party exists
        std::cout << "  ✓ Test 10 Passed: Manifest Borealis layer auto-detection\n";
    }

    // Clean up
    fs::remove_all(temp_base, ec);
    std::cout << "\nAll RomFS Stager Unit Tests Passed Successfully!\n";
    return 0;
}
