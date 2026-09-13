#include "test_common.hpp"
#include <nxdev/env/host_info.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/config/host_config.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/doctor/doctor.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

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
    std::cout << "[Test] Running NXDev Host Environment & Toolchain Test Suite...\n";

    // 1. Test Host Detection (Live and Mocked)
    {
        auto live = nxdev::env::HostInfo::detect();
        NXDEV_TEST_ASSERT(live.os != nxdev::env::OperatingSystem::Unknown);
        NXDEV_TEST_ASSERT(live.arch != nxdev::env::Architecture::Unknown);
        std::cout << "  ✓ Live host detected: " << live.summary() << "\n";

        // Test Mocked WSL2
        std::map<std::string, std::string> wsl_envs{
            {"WSL_DISTRO_NAME", "Ubuntu-22.04"},
            {"WSL_INTEROP", "/run/WSL/1_interop"}
        };
        auto wsl2_mock = nxdev::env::HostInfo::detect_mock(
            nxdev::env::OperatingSystem::Linux,
            nxdev::env::Architecture::X86_64,
            "Linux version 5.15.90.1-microsoft-standard-WSL2 (gcc version 11.2.0)",
            "5.15.90.1-microsoft-standard-WSL2",
            wsl_envs
        );
        NXDEV_TEST_ASSERT(wsl2_mock.is_wsl);
        NXDEV_TEST_ASSERT(wsl2_mock.wsl_version == nxdev::env::WslVersion::Wsl2);
        NXDEV_TEST_ASSERT(wsl2_mock.wsl_distro_name == "Ubuntu-22.04");
        NXDEV_TEST_ASSERT(wsl2_mock.has_wsl_interop);
        NXDEV_TEST_ASSERT(wsl2_mock.is_mounted_windows_path("/mnt/c/Users/Developer/Project"));
        NXDEV_TEST_ASSERT(!wsl2_mock.is_mounted_windows_path("/home/user/project"));
        std::cout << "  ✓ Mocked WSL2 detection passed\n";

        // Test Mocked Windows
        auto win_mock = nxdev::env::HostInfo::detect_mock(
            nxdev::env::OperatingSystem::Windows,
            nxdev::env::Architecture::X86_64,
            "", "", {}
        );
        NXDEV_TEST_ASSERT(win_mock.os == nxdev::env::OperatingSystem::Windows);
        NXDEV_TEST_ASSERT(!win_mock.is_wsl);
        std::cout << "  ✓ Mocked Windows detection passed\n";

        // Test Mocked WSL1
        auto wsl1_mock = nxdev::env::HostInfo::detect_mock(
            nxdev::env::OperatingSystem::Linux,
            nxdev::env::Architecture::X86_64,
            "Linux version 4.4.0-19041-Microsoft (Microsoft@Microsoft.com)",
            "4.4.0-19041-Microsoft",
            {{"WSL_DISTRO_NAME", "Debian"}}
        );
        NXDEV_TEST_ASSERT(wsl1_mock.is_wsl);
        NXDEV_TEST_ASSERT(wsl1_mock.wsl_version == nxdev::env::WslVersion::Wsl1);
        NXDEV_TEST_ASSERT(wsl1_mock.wsl_distro_name == "Debian");
        NXDEV_TEST_ASSERT(!wsl1_mock.has_wsl_interop);
        std::cout << "  ✓ Mocked WSL1 detection passed\n";

        // Test Mocked macOS ARM64
        auto mac_mock = nxdev::env::HostInfo::detect_mock(
            nxdev::env::OperatingSystem::MacOS,
            nxdev::env::Architecture::AArch64,
            "", "", {}
        );
        NXDEV_TEST_ASSERT(mac_mock.os == nxdev::env::OperatingSystem::MacOS);
        NXDEV_TEST_ASSERT(mac_mock.arch == nxdev::env::Architecture::AArch64);
        NXDEV_TEST_ASSERT(!mac_mock.is_wsl);
        std::cout << "  ✓ Mocked macOS ARM64 detection passed\n";
    }

    // 2. Test Process Execution Utility
    {
        auto res = nxdev::exec::ProcessExecutor::execute("echo", {"Hello NXDev", "Argument with space"});
        NXDEV_TEST_ASSERT(res.success);
        NXDEV_TEST_ASSERT(res.exit_code == 0);
        NXDEV_TEST_ASSERT(res.stdout_output.find("Hello NXDev Argument with space") != std::string::npos);

        auto non_existent = nxdev::exec::ProcessExecutor::execute("non_existent_binary_xyz_123", {});
        NXDEV_TEST_ASSERT(!non_existent.success);
        std::cout << "  ✓ Process executor argument array & exit code passed\n";
    }

    // 3. Test Project Model & Discovery (including Spaces & Deep Nested)
    {
        // Upward discovery from child directory
        auto discovered = nxdev::project::NXDevProject::discover(resolve_test_path("examples/manifests/standard/assets"));
        NXDEV_TEST_ASSERT(discovered.has_value());
        NXDEV_TEST_ASSERT(discovered->is_valid());
        NXDEV_TEST_ASSERT(discovered->manifest().application().name == "Standard Homebrew Game");
        NXDEV_TEST_ASSERT(discovered->root_path().find("examples/manifests/standard") != std::string::npos);

        // Explicit path to directory
        auto explicit_dir = nxdev::project::NXDevProject::load_explicit(resolve_test_path("examples/manifests/minimal"));
        NXDEV_TEST_ASSERT(explicit_dir.has_value());
        NXDEV_TEST_ASSERT(explicit_dir->is_valid());
        NXDEV_TEST_ASSERT(explicit_dir->manifest().application().name == "Minimal Switch App");

        // Explicit path to nxapp.yaml file
        auto explicit_file = nxdev::project::NXDevProject::load_explicit(resolve_test_path("examples/manifests/advanced/nxapp.yaml"));
        NXDEV_TEST_ASSERT(explicit_file.has_value());
        NXDEV_TEST_ASSERT(explicit_file->is_valid());
        NXDEV_TEST_ASSERT(explicit_file->manifest().application().name == "Advanced NXDev Engine");

        // Non-existent directory returns nullopt
        auto non_existent = nxdev::project::NXDevProject::load_explicit("/non_existent_directory_xyz");
        NXDEV_TEST_ASSERT(!non_existent.has_value());

        // Test project in temporary directory with spaces in path
        fs::path temp_space_dir = fs::temp_directory_path() / "nxdev test space project";
        fs::create_directories(temp_space_dir / "src" / "deep");
        std::string manifest_content = 
            "schemaVersion: 1\n"
            "application:\n"
            "  name: \"Space Project\"\n"
            "  author: \"Tester\"\n"
            "  version: \"1.0.0\"\n";
        {
            std::ofstream out(temp_space_dir / "nxapp.yaml");
            out << manifest_content;
        }

        auto space_discovered = nxdev::project::NXDevProject::discover((temp_space_dir / "src" / "deep").string());
        NXDEV_TEST_ASSERT(space_discovered.has_value());
        NXDEV_TEST_ASSERT(space_discovered->is_valid());
        NXDEV_TEST_ASSERT(space_discovered->manifest().application().name == "Space Project");

        // Malformed manifest project handling
        fs::path temp_bad_dir = fs::temp_directory_path() / "nxdev test bad project";
        fs::create_directories(temp_bad_dir);
        {
            std::ofstream out(temp_bad_dir / "nxapp.yaml");
            out << "invalid_yaml: [ unclosed";
        }
        auto bad_project = nxdev::project::NXDevProject::load_explicit(temp_bad_dir.string());
        NXDEV_TEST_ASSERT(bad_project.has_value());
        NXDEV_TEST_ASSERT(!bad_project->is_valid());
        NXDEV_TEST_ASSERT(bad_project->diagnostics().has_errors());

        // Cleanup temp dirs
        fs::remove_all(temp_space_dir);
        fs::remove_all(temp_bad_dir);
        std::cout << "  ✓ Project discovery, space paths, and malformed handling passed\n";
    }

    // 4. Test Host Configuration & Precedence
    {
        fs::path temp_cfg_dir = fs::temp_directory_path() / "nxdev_test_config_precedence";
        fs::create_directories(temp_cfg_dir / ".nxdev");
        {
            std::ofstream out(temp_cfg_dir / ".nxdev" / "local.yaml");
            out << "toolchain:\n"
                << "  devkitPro: \"/local/custom/dkp\"\n"
                << "  devkitA64: \"/local/custom/dka64\"\n"
                << "deployment:\n"
                << "  switchIp: \"192.168.1.150\"\n";
        }

        auto cfg = nxdev::config::HostConfig::load(temp_cfg_dir.string());
        NXDEV_TEST_ASSERT(!cfg.global_config_path().empty());
        NXDEV_TEST_ASSERT(cfg.local_config_path() == (temp_cfg_dir / ".nxdev" / "local.yaml").string());
        NXDEV_TEST_ASSERT(cfg.toolchain().devkitpro.has_value() && *cfg.toolchain().devkitpro == "/local/custom/dkp");
        NXDEV_TEST_ASSERT(cfg.toolchain().devkita64.has_value() && *cfg.toolchain().devkita64 == "/local/custom/dka64");
        NXDEV_TEST_ASSERT(cfg.development().switch_ip.has_value() && *cfg.development().switch_ip == "192.168.1.150");

        // Cleanup
        fs::remove_all(temp_cfg_dir);
        std::cout << "  ✓ Host configuration resolution & local.yaml precedence passed\n";
    }

    // 5. Test Environment Toolchain Detection & Mock Hierarchy
    {
        // Setup mock devkitPro directory layout
        fs::path mock_dkp = fs::temp_directory_path() / "nxdev_mock_dkp";
        fs::create_directories(mock_dkp / "devkitA64" / "bin");
        fs::create_directories(mock_dkp / "libnx" / "include");
        fs::create_directories(mock_dkp / "libnx" / "lib");
        fs::create_directories(mock_dkp / "tools" / "bin");
        fs::create_directories(mock_dkp / "portlibs" / "switch" / "include" / "SDL2");
        fs::create_directories(mock_dkp / "portlibs" / "switch" / "lib");

        // Touch mock binaries and headers
        {
            std::ofstream out(mock_dkp / "devkitA64" / "bin" / "aarch64-none-elf-gcc");
            out << "#!/bin/sh\nexit 0\n";
        }
        {
            std::ofstream out(mock_dkp / "devkitA64" / "bin" / "aarch64-none-elf-g++");
            out << "#!/bin/sh\nexit 0\n";
        }
        {
            std::ofstream out(mock_dkp / "libnx" / "include" / "switch.h");
            out << "// mock libnx switch.h\n#define LIBNX_VERSION \"4.6.0\"\n";
        }
        {
            std::ofstream out(mock_dkp / "libnx" / "lib" / "libnx.a");
            out << "mock libnx";
        }
        {
            std::ofstream out(mock_dkp / "portlibs" / "switch" / "include" / "SDL2" / "SDL.h");
            out << "// mock SDL2 header\n";
        }
        {
            std::ofstream out(mock_dkp / "portlibs" / "switch" / "lib" / "libSDL2.a");
            out << "mock libSDL2.a";
        }
        {
            std::ofstream out(mock_dkp / "tools" / "bin" / "elf2nro");
            out << "#!/bin/sh\nexit 0\n";
        }
        {
            std::ofstream out(mock_dkp / "tools" / "bin" / "nacptool");
            out << "#!/bin/sh\nexit 0\n";
        }
        fs::permissions(mock_dkp / "devkitA64" / "bin" / "aarch64-none-elf-gcc",
            fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);
        fs::permissions(mock_dkp / "devkitA64" / "bin" / "aarch64-none-elf-g++",
            fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);
        fs::permissions(mock_dkp / "tools" / "bin" / "elf2nro",
            fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);
        fs::permissions(mock_dkp / "tools" / "bin" / "nacptool",
            fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

        auto cfg = nxdev::config::HostConfig::load();
        auto env = nxdev::env::Environment::detect(cfg, mock_dkp.string());
        NXDEV_TEST_ASSERT(env.devkitpro().is_configured);
        NXDEV_TEST_ASSERT(env.devkitpro().is_valid);
        NXDEV_TEST_ASSERT(env.devkita64().is_configured);
        NXDEV_TEST_ASSERT(env.devkita64().is_valid);
        NXDEV_TEST_ASSERT(env.libnx().found);
        NXDEV_TEST_ASSERT(env.libnx().version == "4.6.0");
        
        auto elf2nro = env.get_tool("elf2nro");
        NXDEV_TEST_ASSERT(elf2nro != nullptr && elf2nro->exists);
        auto nacptool = env.get_tool("nacptool");
        NXDEV_TEST_ASSERT(nacptool != nullptr && nacptool->exists);

        std::string json = env.to_json();
        NXDEV_TEST_ASSERT(json.find("\"isConfigured\": true") != std::string::npos);
        NXDEV_TEST_ASSERT(json.find(mock_dkp.string()) != std::string::npos);
        NXDEV_TEST_ASSERT(json.find("\"version\": \"4.6.0\"") != std::string::npos);

        // Run Doctor against this healthy mock environment
        auto proj = nxdev::project::NXDevProject::load_explicit(resolve_test_path("examples/manifests/standard"));
        nxdev::doctor::Doctor doc;
        doc.run_diagnostics(env, proj, nxdev::doctor::DoctorProfile::All);
        NXDEV_TEST_ASSERT(!doc.checks().empty());
        if (doc.error_count() > 0) {
            std::cerr << "Doctor diagnostics with errors:\n" << doc.to_json() << "\n";
        }
        NXDEV_TEST_ASSERT(doc.error_count() == 0);

        std::string doc_json = doc.to_json();
        NXDEV_TEST_ASSERT(doc_json.find("\"status\":") != std::string::npos);
        NXDEV_TEST_ASSERT(doc_json.find("\"checks\": [") != std::string::npos);
        NXDEV_TEST_ASSERT(doc_json.find("\033[") == std::string::npos);

        // Cleanup mock
        fs::remove_all(mock_dkp);
        std::cout << "  ✓ Mock devkitPro layout, libnx versioning, and Doctor checks passed\n";
    }

    // 6. Test Doctor Severity and Failure Conditions
    {
        // Detect with empty / non-existent mock config paths
        auto cfg = nxdev::config::HostConfig::load();
        auto broken_env = nxdev::env::Environment::detect(cfg, "/non_existent_dkp_path_123", "/non_existent_dka64_path_123");

        nxdev::doctor::Doctor broken_doc;
        broken_doc.run_diagnostics(broken_env, std::nullopt, nxdev::doctor::DoctorProfile::Build);
        NXDEV_TEST_ASSERT(broken_doc.has_errors());
        NXDEV_TEST_ASSERT(broken_doc.error_count() > 0);
        std::cout << "  ✓ Doctor failure severity & error classification passed\n";
    }

    std::cout << "[Test] All NXDev Host Environment & Toolchain tests passed successfully!\n";
    return 0;
}
