#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <nxdev/cli/commands.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/config/host_config.hpp>

namespace fs = std::filesystem;

static void assert_test(bool condition, const std::string& name) {
    if (!condition) {
        std::cerr << "[FAIL] test_prepare: " << name << "\n";
        std::exit(1);
    }
    std::cout << "[PASS] test_prepare: " << name << "\n";
}

int main() {
    std::cout << "=== Running nxdev prepare Unit Tests ===\n";

    auto cfg = nxdev::config::HostConfig::load();
    auto env = nxdev::env::Environment::detect(cfg);

    nxdev::cli::PrepareCommand prep_cmd;

    // Test 1: Help Flag
    {
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"--help"};
        int res = prep_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev prepare --help executes successfully");
    }

    // Test 2: Dry Run Mode
    {
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"--dry-run"};
        int res = prep_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev prepare --dry-run returns 0");
    }

    // Test 3: Check Mode
    {
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"--check", "--json"};
        int res = prep_cmd.execute(args, ctx);
        // Returns 0 if already prepared, or 1 if missing prerequisites
        assert_test(res == 0 || res == 1, "nxdev prepare --check executes cleanly");
    }

    // Test 4: Mock Execution Mode with Mock Home & Shell Profile
    fs::path temp_base = fs::temp_directory_path() / "nxdev_prepare_tests";
    fs::remove_all(temp_base);
    fs::create_directories(temp_base);

    fs::path mock_home = temp_base / "mock_home";
    fs::create_directories(mock_home);
    fs::path mock_bashrc = mock_home / ".bashrc";
    {
        std::ofstream bf(mock_bashrc);
        bf << "# User custom bashrc\nexport MY_VAR=1\n";
    }

    {
        setenv("NXDEV_PREPARE_MOCK", "1", 1);
        setenv("HOME", mock_home.string().c_str(), 1);
        setenv("SHELL", "/bin/bash", 1);

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"--yes", "--json"};
        int res = prep_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev prepare mock execution succeeds");

        // Verify shell configuration
        std::ifstream bf(mock_bashrc);
        std::string content((std::istreambuf_iterator<char>(bf)), std::istreambuf_iterator<char>());
        assert_test(content.find("# >>> NXDev devkitPro >>>") != std::string::npos, "Managed block inserted into .bashrc");
        assert_test(content.find("export DEVKITPRO=\"/opt/devkitpro\"") != std::string::npos, "DEVKITPRO defined in .bashrc");
        assert_test(content.find("export DEVKITA64=\"/opt/devkitpro/devkitA64\"") != std::string::npos, "DEVKITA64 defined in .bashrc");
        assert_test(content.find("export MY_VAR=1") != std::string::npos, "Existing bashrc content preserved");

        // Test 5: Idempotency - Run prepare a second time
        int res2 = prep_cmd.execute(args, ctx);
        assert_test(res2 == 0, "nxdev prepare second run succeeds");

        std::ifstream bf2(mock_bashrc);
        std::string content2((std::istreambuf_iterator<char>(bf2)), std::istreambuf_iterator<char>());

        size_t count = 0;
        std::string tag = "# >>> NXDev devkitPro >>>";
        size_t pos = 0;
        while ((pos = content2.find(tag, pos)) != std::string::npos) {
            ++count;
            pos += tag.length();
        }
        assert_test(count == 1, "Duplicate devkitPro environment block prevented on repeated prepare calls");

        unsetenv("NXDEV_PREPARE_MOCK");
    }

    // Test 6: Custom DEVKITPRO Location Preservation
    {
        fs::path custom_dkp = temp_base / "custom_devkitpro";
        fs::create_directories(custom_dkp / "devkitA64" / "bin");
        fs::create_directories(custom_dkp / "libnx" / "include");
        fs::create_directories(custom_dkp / "tools" / "bin");

        setenv("NXDEV_PREPARE_MOCK", "1", 1);
        setenv("DEVKITPRO", custom_dkp.string().c_str(), 1);
        setenv("DEVKITA64", (custom_dkp / "devkitA64").string().c_str(), 1);

        auto custom_env = nxdev::env::Environment::detect(cfg);

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = custom_env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"--yes", "--json"};
        int res = prep_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev prepare with custom DEVKITPRO succeeds");

        std::ifstream bf(mock_bashrc);
        std::string content((std::istreambuf_iterator<char>(bf)), std::istreambuf_iterator<char>());
        assert_test(content.find("export DEVKITPRO=\"" + custom_dkp.string() + "\"") != std::string::npos, "Custom DEVKITPRO path preserved in profile");
        assert_test(content.find("export DEVKITA64=\"" + (custom_dkp / "devkitA64").string() + "\"") != std::string::npos, "Custom DEVKITA64 path preserved in profile");

        unsetenv("NXDEV_PREPARE_MOCK");
        unsetenv("DEVKITPRO");
        unsetenv("DEVKITA64");
    }

    // Test 7: Conflict Detection & Repair
    {
        fs::path custom_dkp = temp_base / "conflict_dkp";
        fs::path mismatch_dka = temp_base / "other_dka";
        fs::create_directories(custom_dkp);
        fs::create_directories(mismatch_dka);

        setenv("DEVKITPRO", custom_dkp.string().c_str(), 1);
        setenv("DEVKITA64", mismatch_dka.string().c_str(), 1);

        auto conflict_env = nxdev::env::Environment::detect(cfg);
        assert_test(!conflict_env.devkita64().is_consistent, "Environment detects inconsistent DEVKITA64 / DEVKITPRO");

        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = conflict_env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        // Without repair/yes, should detect conflict
        std::vector<std::string> args = {"--json"};
        int res = prep_cmd.execute(args, ctx);
        assert_test(res == 1, "nxdev prepare detects conflict without repair flag");

        // With repair, should resolve conflict
        setenv("NXDEV_PREPARE_MOCK", "1", 1);
        std::vector<std::string> repair_args = {"--repair", "--yes", "--json"};
        int rep_res = prep_cmd.execute(repair_args, ctx);
        assert_test(rep_res == 0, "nxdev prepare --repair resolves inconsistent environment variables");

        unsetenv("NXDEV_PREPARE_MOCK");
        unsetenv("DEVKITPRO");
        unsetenv("DEVKITA64");
    }

    // Clean up
    fs::remove_all(temp_base);

    std::cout << "\n✓ All nxdev prepare unit tests passed successfully!\n";
    return 0;
}
