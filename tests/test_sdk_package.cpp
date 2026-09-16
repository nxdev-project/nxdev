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
        std::cerr << "[FAIL] test_sdk_package: " << name << "\n";
        std::exit(1);
    }
    std::cout << "[PASS] test_sdk_package: " << name << "\n";
}

int main() {
    std::cout << "=== Running NXDevSDK Packaging Unit Tests ===\n";

    fs::path temp_base = fs::temp_directory_path() / "nxdev_sdk_pack_tests";
    fs::remove_all(temp_base);
    fs::create_directories(temp_base);

    auto cfg = nxdev::config::HostConfig::load();
    auto env = nxdev::env::Environment::detect(cfg);

    nxdev::cli::SdkCommand sdk_cmd;

    // Test 1: Sdk Info
    {
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"info", "--json"};
        int res = sdk_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev sdk info --json executes successfully");
    }

    // Test 2: Sdk Package Archive Generation
    fs::path out_sdk_dir = temp_base / "dist_sdk";
    fs::create_directories(out_sdk_dir);
    {
        nxdev::cli::CommandContext ctx{
            .project = std::nullopt,
            .env = env,
            .config = cfg,
            .explicit_project_path = "",
            .verbose = false,
            .no_color = true
        };

        std::vector<std::string> args = {"package", "--output", out_sdk_dir.string(), "--json"};
        int res = sdk_cmd.execute(args, ctx);
        assert_test(res == 0, "nxdev sdk package generates archive successfully");
        assert_test(fs::exists(out_sdk_dir / "NXDevSDK.zip"), "NXDevSDK.zip exists in output directory");
    }

    // Test 3: Outer and Inner Archive Inspection
    fs::path extract_dir = temp_base / "extracted_outer";
    fs::create_directories(extract_dir);
    {
        std::string unz_cmd = "unzip -q \"" + (out_sdk_dir / "NXDevSDK.zip").string() + "\" -d \"" + extract_dir.string() + "\"";
        int res = std::system(unz_cmd.c_str());
        assert_test(res == 0, "Unzip NXDevSDK.zip succeeds");
        assert_test(fs::exists(extract_dir / "install.sh"), "install.sh present at root of NXDevSDK.zip");
        assert_test(fs::exists(extract_dir / "SDK.zip"), "SDK.zip present at root of NXDevSDK.zip");

        // Inspect SDK.zip
        fs::path inner_extract = temp_base / "extracted_inner";
        fs::create_directories(inner_extract);
        std::string unz_inner_cmd = "unzip -q \"" + (extract_dir / "SDK.zip").string() + "\" -d \"" + inner_extract.string() + "\"";
        res = std::system(unz_inner_cmd.c_str());
        assert_test(res == 0, "Unzip SDK.zip succeeds");

        assert_test(fs::exists(inner_extract / "include" / "nxdev" / "nxdev.hpp"), "SDK headers present in SDK.zip");
        assert_test(fs::exists(inner_extract / "share" / "nxdev" / "sdk.json"), "sdk.json version metadata present");
        assert_test(fs::exists(inner_extract / "share" / "nxdev" / "cmake" / "NXDevConfig.cmake"), "NXDevConfig.cmake present");
        assert_test(fs::exists(inner_extract / "licenses" / "LICENSE"), "LICENSE present");
        assert_test(fs::exists(inner_extract / "bin" / "hacbrewpack"), "hacbrewpack binary present in SDK.zip bin/");
    }

    // Test 4: Run install.sh to Temporary Custom Prefix with Mock Shell Profile
    fs::path install_prefix = temp_base / "installed_sdk";
    fs::path mock_home = temp_base / "mock_user_home";
    fs::create_directories(mock_home);
    fs::path mock_bashrc = mock_home / ".bashrc";
    {
        std::ofstream bf(mock_bashrc);
        bf << "# Existing User Config\nexport FOO=bar\n";
    }

    {
        std::string inst_cmd = "HOME=\"" + mock_home.string() + "\" " +
                               (extract_dir / "install.sh").string() +
                               " --prefix \"" + install_prefix.string() + "\" -y";
        int res = std::system(inst_cmd.c_str());
        assert_test(res == 0, "install.sh runs cleanly to custom prefix");

        assert_test(fs::exists(install_prefix / "include" / "nxdev" / "nxdev.hpp"), "Headers installed to prefix");
        assert_test(fs::exists(install_prefix / "share" / "nxdev" / "sdk.json"), "sdk.json installed to prefix");
        assert_test(fs::exists(install_prefix / "share" / "nxdev" / "cmake" / "NXDevConfig.cmake"), "CMake config installed to prefix");
        assert_test(fs::exists(install_prefix / "bin"), "bin directory installed to prefix");
        assert_test(fs::exists(install_prefix / "bin" / "hacbrewpack"), "hacbrewpack binary installed to prefix bin/");
    }

    // Test 5: Verify Managed Block in Shell Profile & Idempotence
    {
        std::ifstream bf1(mock_bashrc);
        std::string content1((std::istreambuf_iterator<char>(bf1)), std::istreambuf_iterator<char>());
        assert_test(content1.find("# >>> NXDevSDK >>>") != std::string::npos, "Managed block inserted into .bashrc");
        assert_test(content1.find("export NXDEV_SDK_ROOT=\"" + install_prefix.string() + "\"") != std::string::npos, "NXDEV_SDK_ROOT configured in .bashrc");
        assert_test(content1.find("export FOO=bar") != std::string::npos, "Existing user configuration preserved");

        // Run installer a second time to same prefix
        std::string inst_cmd2 = "HOME=\"" + mock_home.string() + "\" " +
                                (extract_dir / "install.sh").string() +
                                " --prefix \"" + install_prefix.string() + "\" -f -y";
        int res2 = std::system(inst_cmd2.c_str());
        assert_test(res2 == 0, "install.sh second run succeeds");

        std::ifstream bf2(mock_bashrc);
        std::string content2((std::istreambuf_iterator<char>(bf2)), std::istreambuf_iterator<char>());

        // Count occurrences of managed block header
        size_t occurrences = 0;
        std::string tag = "# >>> NXDevSDK >>>";
        size_t pos = 0;
        while ((pos = content2.find(tag, pos)) != std::string::npos) {
            ++occurrences;
            pos += tag.length();
        }
        assert_test(occurrences == 1, "Duplicate environment block prevented on repeated installer runs");
    }

    // Test 6: Verify --no-env Flag
    fs::path no_env_home = temp_base / "no_env_home";
    fs::create_directories(no_env_home);
    fs::path no_env_bashrc = no_env_home / ".bashrc";
    {
        std::ofstream bf(no_env_bashrc);
        bf << "# Unmodified bashrc\n";
    }
    {
        std::string no_env_cmd = "HOME=\"" + no_env_home.string() + "\" " +
                                 (extract_dir / "install.sh").string() +
                                 " --prefix \"" + (temp_base / "sdk_no_env").string() + "\" --no-env -y";
        int res = std::system(no_env_cmd.c_str());
        assert_test(res == 0, "install.sh with --no-env executes cleanly");

        std::ifstream bf(no_env_bashrc);
        std::string content((std::istreambuf_iterator<char>(bf)), std::istreambuf_iterator<char>());
        assert_test(content.find("# >>> NXDevSDK >>>") == std::string::npos, "--no-env leaves shell configuration untouched");
    }

    // Test 7: Precedence and Detection of NXDEV_SDK_ROOT
    {
        // Set NXDEV_SDK_ROOT in environment
        setenv("NXDEV_SDK_ROOT", install_prefix.string().c_str(), 1);

        auto installed_env = nxdev::env::Environment::detect(cfg);
        assert_test(installed_env.sdk().found, "Environment detects SDK via NXDEV_SDK_ROOT");
        assert_test(installed_env.sdk().is_valid, "Detected SDK is valid");
        assert_test(installed_env.sdk().env_root_set, "env_root_set is true when NXDEV_SDK_ROOT is present");
        assert_test(installed_env.sdk().path == install_prefix.string(), "Detected SDK path matches NXDEV_SDK_ROOT");
        assert_test(installed_env.sdk().has_cmake_package, "CMake configuration package detected in installed SDK");

        // Clean environment variable
        unsetenv("NXDEV_SDK_ROOT");
    }

    // Clean up
    fs::remove_all(temp_base);

    std::cout << "\n✓ All NXDevSDK packaging and environment unit tests passed successfully!\n";
    return 0;
}
