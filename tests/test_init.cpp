#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <nxdev/cli/commands.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/config/host_config.hpp>

namespace fs = std::filesystem;

static void assert_test(bool condition, const std::string& name) {
    if (!condition) {
        std::cerr << "[FAIL] test_init: " << name << "\n";
        std::exit(1);
    }
    std::cout << "[PASS] test_init: " << name << "\n";
}

int main() {
    std::cout << "=== Running nxdev init Unit Tests ===\n";

    fs::path temp_base = fs::temp_directory_path() / "nxdev_init_tests";
    fs::remove_all(temp_base);
    fs::create_directories(temp_base);

    auto cfg = nxdev::config::HostConfig::load();
    auto env = nxdev::env::Environment::detect(cfg);

    nxdev::cli::InitCommand init_cmd;

    // Test 1: Empty Directory Init
    {
        fs::path p1 = temp_base / "empty_proj";
        fs::create_directories(p1);

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p1.string(), "--name", "Empty Test App", "--author", "Tester", "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "Empty directory initialization");
        assert_test(fs::exists(p1 / "nxapp.yaml"), "nxapp.yaml created");
        assert_test(fs::exists(p1 / "CMakeLists.txt"), "CMakeLists.txt created");
        assert_test(fs::exists(p1 / "src" / "main.cpp"), "src/main.cpp created for empty directory");

        auto proj = nxdev::project::NXDevProject::load_explicit(p1.string());
        assert_test(proj.has_value() && proj->is_valid(), "Initialized project passes validation");
    }

    // Test 2: Existing Source Directory (C project)
    {
        fs::path p2 = temp_base / "c_proj";
        fs::create_directories(p2 / "src");
        {
            std::ofstream src(p2 / "src" / "main.c");
            src << "#include <stdio.h>\nint main() { return 0; }\n";
        }

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p2.string(), "--name", "C Test App", "--author", "C Dev", "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "Existing C source project initialization");
        assert_test(fs::exists(p2 / "nxapp.yaml"), "C project nxapp.yaml created");
        assert_test(fs::exists(p2 / "CMakeLists.txt"), "C project CMakeLists.txt created");

        // Verify C language detected
        std::ifstream mf(p2 / "nxapp.yaml");
        std::string mfc((std::istreambuf_iterator<char>(mf)), std::istreambuf_iterator<char>());
        assert_test(mfc.find("language: c") != std::string::npos, "Language detected as C");
    }

    // Test 3: Existing CMakeLists.txt Preservation
    {
        fs::path p3 = temp_base / "cmake_proj";
        fs::create_directories(p3 / "src");
        {
            std::ofstream cm(p3 / "CMakeLists.txt");
            cm << "# Custom User CMake Configuration\nproject(custom_user_proj)\n";
        }
        {
            std::ofstream src(p3 / "src" / "main.cpp");
            src << "int main() { return 0; }\n";
        }

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p3.string(), "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "Initialization with existing CMakeLists.txt");

        std::ifstream cm_in(p3 / "CMakeLists.txt");
        std::string cm_content((std::istreambuf_iterator<char>(cm_in)), std::istreambuf_iterator<char>());
        assert_test(cm_content.find("Custom User CMake Configuration") != std::string::npos, "Existing CMakeLists.txt preserved");
    }

    // Test 4: Existing Makefile (devkitPro project)
    {
        fs::path p4 = temp_base / "dkp_proj";
        fs::create_directories(p4 / "source");
        {
            std::ofstream mf(p4 / "Makefile");
            mf << "# devkitPro Makefile for switch\ninclude $(DEVKITPRO)/libnx/switch_rules\n";
        }
        {
            std::ofstream src(p4 / "source" / "main.cpp");
            src << "#include <switch.h>\nint main() { return 0; }\n";
        }

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p4.string(), "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "devkitPro Makefile project initialization");
        assert_test(fs::exists(p4 / "nxapp.yaml"), "nxapp.yaml created alongside Makefile");
        assert_test(fs::exists(p4 / "Makefile"), "Makefile preserved");
    }

    // Test 5: Idempotency (Already Initialized)
    {
        fs::path p1 = temp_base / "empty_proj";
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p1.string(), "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "Already initialized directory reports success without overwrite");
    }

    // Test 6: Path with spaces
    {
        fs::path p_spaces = temp_base / "Path With Spaces";
        fs::create_directories(p_spaces);

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {p_spaces.string(), "--json"};
        int res = init_cmd.execute(args, ctx);
        assert_test(res == 0, "Initialization in path with spaces");
        assert_test(fs::exists(p_spaces / "nxapp.yaml"), "nxapp.yaml created in spaced path");
    }

    // Clean up
    fs::remove_all(temp_base);

    std::cout << "\n✓ All nxdev init unit tests passed successfully!\n";
    return 0;
}
