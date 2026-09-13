#include "test_common.hpp"
#include <nxdev/build/build_orchestrator.hpp>
#include <nxdev/env/environment.hpp>
#include <nxdev/config/host_config.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/exec/process.hpp>
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
    std::cout << "[Test] Running NXDev Switch Build Pipeline & Orchestrator Test Suite...\n";

    // 1. Test Profile Parsing and Representation
    {
        NXDEV_TEST_ASSERT(nxdev::build::build_profile_to_string(nxdev::build::BuildProfile::Debug) == "debug");
        NXDEV_TEST_ASSERT(nxdev::build::build_profile_to_string(nxdev::build::BuildProfile::Release) == "release");

        auto p_dbg = nxdev::build::parse_build_profile("debug");
        NXDEV_TEST_ASSERT(p_dbg.has_value() && *p_dbg == nxdev::build::BuildProfile::Debug);
        auto p_rel = nxdev::build::parse_build_profile("Release");
        NXDEV_TEST_ASSERT(p_rel.has_value() && *p_rel == nxdev::build::BuildProfile::Release);
        auto p_inv = nxdev::build::parse_build_profile("invalid_profile");
        NXDEV_TEST_ASSERT(!p_inv.has_value());
        std::cout << "  ✓ Build profile mapping and parsing passed\n";
    }

    // 2. Test Build Directory Resolution
    {
        std::string bdir = nxdev::build::BuildOrchestrator::resolve_build_directory("/my/project", "debug");
        NXDEV_TEST_ASSERT(bdir == "/my/project/.nxdev/build/debug");
        std::cout << "  ✓ Build directory resolution passed\n";
    }

    // 3. Test Configure Arguments Generation
    {
        auto proj = nxdev::project::NXDevProject::load_explicit(resolve_test_path("examples/hello-switch"));
        NXDEV_TEST_ASSERT(proj.has_value());
        NXDEV_TEST_ASSERT(proj->is_valid());

        auto cfg = nxdev::config::HostConfig::load(proj->root_path());
        auto env = nxdev::env::Environment::detect(cfg);

        nxdev::build::BuildOptions opts;
        opts.profile = nxdev::build::BuildProfile::Debug;

        auto args = nxdev::build::BuildOrchestrator::generate_configure_args(*proj, env, opts, "/tmp/nxdev_test_build");
        
        bool has_source = false;
        bool has_build_dir = false;
        bool has_toolchain = false;
        bool has_build_type = false;
        bool has_export_cc = false;

        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-S" && i + 1 < args.size() && args[i + 1] == proj->root_path()) has_source = true;
            if (args[i] == "-B" && i + 1 < args.size() && args[i + 1] == "/tmp/nxdev_test_build") has_build_dir = true;
            if (args[i].find("-DCMAKE_TOOLCHAIN_FILE=") != std::string::npos) has_toolchain = true;
            if (args[i] == "-DCMAKE_BUILD_TYPE=Debug") has_build_type = true;
            if (args[i] == "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON") has_export_cc = true;
        }

        NXDEV_TEST_ASSERT(has_source);
        NXDEV_TEST_ASSERT(has_build_dir);
        NXDEV_TEST_ASSERT(has_toolchain);
        NXDEV_TEST_ASSERT(has_build_type);
        NXDEV_TEST_ASSERT(has_export_cc);
        std::cout << "  ✓ CMake configure argument generation passed\n";
    }

    // 4. Test Safe Clean Path Validation
    {
        nxdev::project::NXDevProject invalid_proj;
        std::string err;
        NXDEV_TEST_ASSERT(!nxdev::build::BuildOrchestrator::safe_clean(invalid_proj, err));
        std::cout << "  ✓ Safe clean path protection passed\n";
    }

    // 5. Test BuildResult JSON Serialization
    {
        nxdev::build::BuildResult res;
        res.success = true;
        res.exit_code = 0;
        res.profile = "debug";
        res.project_root = "/mock/root";
        res.build_directory = "/mock/root/.nxdev/build/debug";
        res.target_name = "hello-switch";
        res.elf_path = "/mock/root/.nxdev/build/debug/bin/hello-switch.elf";
        res.compile_commands_path = "/mock/root/.nxdev/build/debug/compile_commands.json";

        std::string json = res.to_json();
        NXDEV_TEST_ASSERT(json.find("\"status\": \"success\"") != std::string::npos);
        NXDEV_TEST_ASSERT(json.find("\"type\": \"elf\"") != std::string::npos);
        NXDEV_TEST_ASSERT(json.find("hello-switch.elf") != std::string::npos);
        NXDEV_TEST_ASSERT(json.find("\033[") == std::string::npos);
        std::cout << "  ✓ BuildResult JSON serialization passed\n";
    }

    // 6. Integration Test: Live Switch Target Build (when devkitA64 is present)
    {
        auto proj = nxdev::project::NXDevProject::load_explicit(resolve_test_path("examples/hello-switch"));
        NXDEV_TEST_ASSERT(proj.has_value());

        auto cfg = nxdev::config::HostConfig::load(proj->root_path());
        auto env = nxdev::env::Environment::detect(cfg);

        if (env.devkita64().is_valid && env.libnx().found) {
            std::cout << "  -> devkitA64 toolchain detected on host. Executing live Switch compilation...\n";

            // Configure
            nxdev::build::BuildOptions config_opts;
            config_opts.profile = nxdev::build::BuildProfile::Debug;
            config_opts.fresh_configure = true;

            auto config_res = nxdev::build::BuildOrchestrator::configure(*proj, env, config_opts);
            NXDEV_TEST_ASSERT(config_res.success);
            NXDEV_TEST_ASSERT(config_res.exit_code == 0);
            std::cout << "  ✓ Live CMake configure succeeded\n";

            // Build
            nxdev::build::BuildOptions build_opts;
            build_opts.profile = nxdev::build::BuildProfile::Debug;

            auto build_res = nxdev::build::BuildOrchestrator::build(*proj, env, build_opts);
            NXDEV_TEST_ASSERT(build_res.success);
            NXDEV_TEST_ASSERT(build_res.exit_code == 0);
            NXDEV_TEST_ASSERT(!build_res.elf_path.empty());
            NXDEV_TEST_ASSERT(fs::exists(build_res.elf_path));
            std::cout << "  ✓ Live CMake build produced ELF: " << build_res.elf_path << "\n";

            // Inspect the generated ELF binary header (ELF64, Little Endian, AArch64)
            std::ifstream elf_file(build_res.elf_path, std::ios::binary);
            NXDEV_TEST_ASSERT(elf_file.is_open());

            unsigned char e_ident[16];
            elf_file.read(reinterpret_cast<char*>(e_ident), 16);
            NXDEV_TEST_ASSERT(e_ident[0] == 0x7F && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F'); // Magic
            NXDEV_TEST_ASSERT(e_ident[4] == 2); // ELFCLASS64
            NXDEV_TEST_ASSERT(e_ident[5] == 1); // ELFDATA2LSB (Little Endian)

            // Read e_machine (offset 18 in 64-bit ELF header)
            uint16_t e_machine = 0;
            elf_file.seekg(18);
            elf_file.read(reinterpret_cast<char*>(&e_machine), sizeof(e_machine));
            NXDEV_TEST_ASSERT(e_machine == 183); // EM_AARCH64 = 183 (0x00B7)
            std::cout << "  ✓ Verified output binary is a valid 64-bit AArch64 ELF (EM_AARCH64)\n";

            // Verify compile_commands.json
            NXDEV_TEST_ASSERT(fs::exists(fs::path(proj->root_path()) / ".nxdev" / "compile_commands.json"));
            std::cout << "  ✓ Verified .nxdev/compile_commands.json generation\n";

            // Test clean
            std::string clean_err;
            NXDEV_TEST_ASSERT(nxdev::build::BuildOrchestrator::safe_clean(*proj, clean_err));
            NXDEV_TEST_ASSERT(!fs::exists(fs::path(proj->root_path()) / ".nxdev" / "build"));
            std::cout << "  ✓ Verified safe clean of build directory\n";
        } else {
            std::cout << "  (devkitA64 not available on this host: live compilation skipped)\n";
        }
    }

    std::cout << "[Test] All NXDev Switch Build Pipeline tests passed successfully!\n";
    return 0;
}
