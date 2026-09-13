#include "test_common.hpp"
#include <nxdev/pack/pack.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/nsp_backend.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

static void create_dummy_elf_aarch64(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    unsigned char header[64] = {0};
    header[0] = 0x7F; header[1] = 'E'; header[2] = 'L'; header[3] = 'F'; // Magic
    header[4] = 2; // 64-bit
    header[5] = 1; // Little endian
    header[6] = 1; // Version
    header[18] = 183; header[19] = 0; // EM_AARCH64 = 183 (0x00B7)
    out.write(reinterpret_cast<char*>(header), sizeof(header));
}

static void create_dummy_elf_x86_64(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    unsigned char header[64] = {0};
    header[0] = 0x7F; header[1] = 'E'; header[2] = 'L'; header[3] = 'F';
    header[4] = 2;
    header[5] = 1;
    header[6] = 1;
    header[18] = 62; header[19] = 0; // EM_X86_64 = 62
    out.write(reinterpret_cast<char*>(header), sizeof(header));
}

static void create_dummy_jpeg(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    unsigned char jpeg_hdr[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00};
    out.write(reinterpret_cast<char*>(jpeg_hdr), sizeof(jpeg_hdr));
    // Pad
    char zero[100] = {0};
    out.write(zero, sizeof(zero));
}

static void create_dummy_png(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    unsigned char png_hdr[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    out.write(reinterpret_cast<char*>(png_hdr), sizeof(png_hdr));
}

static void create_dummy_nro(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    char buf[1024] = {0};
    // Magic 'NRO0' at offset 0x10 (16)
    buf[0x10] = 'N'; buf[0x11] = 'R'; buf[0x12] = 'O'; buf[0x13] = '0';
    out.write(buf, sizeof(buf));
}

void test_sanitize_filename() {
    using nxdev::pack::NroPackBackend;
    NXDEV_TEST_ASSERT(NroPackBackend::sanitize_filename("Hello World") == "hello-world");
    NXDEV_TEST_ASSERT(NroPackBackend::sanitize_filename("My Switch App! 🎮") == "my-switch-app");
    NXDEV_TEST_ASSERT(NroPackBackend::sanitize_filename("---test---") == "test");
    NXDEV_TEST_ASSERT(NroPackBackend::sanitize_filename("MyApp_1.2.3") == "myapp_1-2-3");
    NXDEV_TEST_ASSERT(NroPackBackend::sanitize_filename("   ") == "app");
}

void test_jpeg_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_jpeg";
    fs::create_directories(temp_dir);

    std::string valid_jpg = (temp_dir / "icon.jpg").string();
    std::string invalid_png = (temp_dir / "icon.png").string();
    std::string bad_file = (temp_dir / "empty.jpg").string();

    create_dummy_jpeg(valid_jpg);
    create_dummy_png(invalid_png);
    { std::ofstream out(bad_file); out << "not a jpeg"; }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::NroPackBackend::is_valid_jpeg(valid_jpg, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::is_valid_jpeg(invalid_png, err));
    NXDEV_TEST_ASSERT(err.find("PNG format") != std::string::npos);

    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::is_valid_jpeg(bad_file, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::is_valid_jpeg((temp_dir / "nonexistent.jpg").string(), err));

    fs::remove_all(temp_dir);
}

void test_elf_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_elf";
    fs::create_directories(temp_dir);

    std::string valid_aarch64 = (temp_dir / "app_aarch64.elf").string();
    std::string x86_elf = (temp_dir / "app_x86.elf").string();
    std::string bad_elf = (temp_dir / "corrupt.elf").string();

    create_dummy_elf_aarch64(valid_aarch64);
    create_dummy_elf_x86_64(x86_elf);
    { std::ofstream out(bad_elf); out << "corrupt binary data"; }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::NroPackBackend::validate_elf_binary(valid_aarch64, err));
    
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_elf_binary(x86_elf, err));
    NXDEV_TEST_ASSERT(err.find("not AArch64") != std::string::npos);

    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_elf_binary(bad_elf, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_elf_binary((temp_dir / "missing.elf").string(), err));

    fs::remove_all(temp_dir);
}

void test_nro_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_nro";
    fs::create_directories(temp_dir);

    std::string valid_nro = (temp_dir / "app.nro").string();
    std::string bad_nro = (temp_dir / "bad.nro").string();

    create_dummy_nro(valid_nro);
    { std::ofstream out(bad_nro); out << "not an nro file"; }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::NroPackBackend::validate_nro_binary(valid_nro, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_nro_binary(bad_nro, err));

    fs::remove_all(temp_dir);
}

void test_nro_packaging_mock_tools() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_pack_mock_test";
    fs::create_directories(temp_dir / "assets");
    fs::create_directories(temp_dir / "romfs");
    fs::create_directories(temp_dir / "tools");

    std::string elf_path = (temp_dir / "app.elf").string();
    std::string icon_path = (temp_dir / "assets" / "icon.jpg").string();
    std::string romfs_dir = (temp_dir / "romfs").string();

    create_dummy_elf_aarch64(elf_path);
    create_dummy_jpeg(icon_path);
    { std::ofstream out(temp_dir / "romfs" / "file.txt"); out << "Hello from romfs"; }

    // Create mock nacptool
    std::string mock_nacptool = (temp_dir / "tools" / "nacptool").string();
    {
        std::ofstream out(mock_nacptool);
        out << "#!/bin/sh\n"
            << "for arg in \"$@\"; do\n"
            << "  case $arg in\n"
            << "    *.nacp) OUT=$arg ;;\n"
            << "  esac\n"
            << "done\n"
            << "echo \"mock nacp metadata\" > \"$OUT\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_nacptool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Create mock elf2nro
    std::string mock_elf2nro = (temp_dir / "tools" / "elf2nro").string();
    {
        std::ofstream out(mock_elf2nro);
        out << "#!/bin/sh\n"
            << "OUT=$2\n"
            << "# Write NRO0 header at offset 16\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(1024); b[16:20]=b\"NRO0\"; f.write(b); f.close()' \"$OUT\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_elf2nro, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Setup request
    nxdev::pack::PackageRequest req;
    req.manifest.application().name = "Mock App";
    req.manifest.application().author = "Mock Dev";
    req.manifest.application().version = "2.0.0";
    req.manifest.application().title_id = "0100000000000099";
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "assets/icon.jpg";
    req.manifest.assets().romfs.enabled = true;
    req.manifest.assets().romfs.raw_path = "romfs";
    req.project_root = temp_dir.string();
    req.input_elf_path = elf_path;
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NRO;
    req.nacptool_path_override = mock_nacptool;
    req.elf2nro_path_override = mock_elf2nro;

    std::vector<nxdev::pack::PackageStage> recorded_stages;
    auto progress_cb = [&](nxdev::pack::PackageStage stage, std::string_view) {
        recorded_stages.push_back(stage);
    };

    nxdev::pack::PackManager manager;
    auto res = manager.pack(req, nullptr, progress_cb);

    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.format == nxdev::pack::PackageFormat::NRO);
    NXDEV_TEST_ASSERT(res.file_size_bytes == 1024);
    NXDEV_TEST_ASSERT(res.icon_source == "project");
    NXDEV_TEST_ASSERT(fs::exists(res.output_file));
    NXDEV_TEST_ASSERT(res.output_file.find("mock-app.nro") != std::string::npos);

    // Check JSON serialization
    std::string json = res.to_json();
    NXDEV_TEST_ASSERT(json.find("\"status\": \"success\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"format\": \"nro\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"iconSource\": \"project\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\033[") == std::string::npos); // No ANSI escape codes

    NXDEV_TEST_ASSERT(!recorded_stages.empty());
    NXDEV_TEST_ASSERT(recorded_stages[0] == nxdev::pack::PackageStage::Preflight);

    fs::remove_all(temp_dir);
}

void test_nro_pack_error_conditions() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_pack_err_test";
    fs::create_directories(temp_dir);

    // Create a mock nacptool that creates the output file
    std::string mock_nacp = (temp_dir / "mock_nacptool.sh").string();
    {
        std::ofstream out(mock_nacp);
        out << "#!/bin/sh\n"
            << "for arg in \"$@\"; do\n"
            << "  case $arg in\n"
            << "    *.nacp) OUT=$arg ;;\n"
            << "  esac\n"
            << "done\n"
            << "echo \"mock nacp\" > \"$OUT\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_nacp, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    nxdev::pack::PackageRequest req;
    req.project_root = temp_dir.string();
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NRO;
    req.input_elf_path = (temp_dir / "nonexistent.elf").string();
    req.nacptool_path_override = mock_nacp;
    req.elf2nro_path_override = "/bin/true";

    nxdev::pack::PackManager manager;

    // 1. Missing ELF binary
    auto res_no_elf = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_no_elf.success);
    NXDEV_TEST_ASSERT(res_no_elf.error_code == nxdev::pack::PackErrorCode::InvalidInputBinary);

    // Create ELF
    std::string elf_path = (temp_dir / "app.elf").string();
    create_dummy_elf_aarch64(elf_path);
    req.input_elf_path = elf_path;

    // 2. Non-JPEG icon
    std::string png_icon = (temp_dir / "icon.png").string();
    create_dummy_png(png_icon);
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "icon.png";
    auto res_bad_icon = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_bad_icon.success);
    NXDEV_TEST_ASSERT(res_bad_icon.error_code == nxdev::pack::PackErrorCode::InvalidIcon);
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::LibnxDefault;
    req.manifest.assets().icon.raw_path.clear();

    // 3. Missing RomFS directory
    req.manifest.assets().romfs.enabled = true;
    req.manifest.assets().romfs.raw_path = "nonexistent_romfs";
    auto res_bad_romfs = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_bad_romfs.success);
    NXDEV_TEST_ASSERT(res_bad_romfs.error_code == nxdev::pack::PackErrorCode::InvalidRomFS);
    req.manifest.assets().romfs.enabled = false;
    req.manifest.assets().romfs.raw_path.clear();

    // 4. Failing nacptool
    std::string fail_tool = (temp_dir / "fail_tool.sh").string();
    {
        std::ofstream out(fail_tool);
        out << "#!/bin/sh\necho 'mock error' >&2\nexit 1\n";
    }
    fs::permissions(fail_tool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    req.nacptool_path_override = fail_tool;
    auto res_fail_nacp = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_fail_nacp.success);
    NXDEV_TEST_ASSERT(res_fail_nacp.error_code == nxdev::pack::PackErrorCode::NacpGenerationFailed);

    fs::remove_all(temp_dir);
}

static void create_dummy_pfs0(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    char buf[512] = {0};
    buf[0] = 'P'; buf[1] = 'F'; buf[2] = 'S'; buf[3] = '0';
    uint32_t num_files = 1;
    std::memcpy(&buf[4], &num_files, sizeof(uint32_t));
    out.write(buf, sizeof(buf));
}

void test_npdm_generation() {
    using nxdev::pack::HacBrewPackAdapter;
    nxdev::manifest::Manifest manifest;
    manifest.application().name = "Test App";
    manifest.application().title_id = "0100000000000099";
    manifest.npdm().preset = nxdev::manifest::NpdmPreset::Standard;
    manifest.npdm().main_thread.priority = 44;
    manifest.npdm().main_thread.core = 0;
    manifest.npdm().main_thread.stack_size = 0x40000;

    std::string json, err;
    NXDEV_TEST_ASSERT(HacBrewPackAdapter::generate_npdm_json(manifest, json, err));
    NXDEV_TEST_ASSERT(json.find("\"title_id\": \"0x0100000000000099\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"main_thread_priority\": 44") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"sm:\"") != std::string::npos);

    // Network preset
    manifest.npdm().preset = nxdev::manifest::NpdmPreset::Network;
    NXDEV_TEST_ASSERT(HacBrewPackAdapter::generate_npdm_json(manifest, json, err));
    NXDEV_TEST_ASSERT(json.find("\"sfdnsres\"") != std::string::npos);

    // Invalid priority
    manifest.npdm().main_thread.priority = 100;
    NXDEV_TEST_ASSERT(!HacBrewPackAdapter::generate_npdm_json(manifest, json, err));
    NXDEV_TEST_ASSERT(err.find("priority") != std::string::npos);
}

void test_key_resolution() {
    using nxdev::pack::HacBrewPackAdapter;
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_keys";
    fs::create_directories(temp_dir);

    std::string custom_keys = (temp_dir / "custom.keys").string();
    { std::ofstream out(custom_keys); out << "dummy_key_data"; }

    nxdev::pack::PackageRequest req;
    req.keys_path = custom_keys;

    // 1. Explicit request path
    std::string resolved = HacBrewPackAdapter::resolve_keys_file(req);
    NXDEV_TEST_ASSERT(resolved == custom_keys);

    // 2. Project-local .nxdev/prod.keys
    req.keys_path.clear();
    fs::create_directories(temp_dir / ".nxdev");
    std::string proj_keys = (temp_dir / ".nxdev" / "prod.keys").string();
    { std::ofstream out(proj_keys); out << "proj_keys"; }
    req.project_root = temp_dir.string();

    resolved = HacBrewPackAdapter::resolve_keys_file(req);
    NXDEV_TEST_ASSERT(resolved == proj_keys);

    fs::remove_all(temp_dir);
}

void test_pfs0_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_pfs0";
    fs::create_directories(temp_dir);

    std::string valid_nsp = (temp_dir / "valid.nsp").string();
    std::string bad_nsp = (temp_dir / "bad.nsp").string();

    create_dummy_pfs0(valid_nsp);
    { std::ofstream out(bad_nsp); out << "not a pfs0 binary"; }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::HacBrewPackAdapter::validate_pfs0_binary(valid_nsp, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_pfs0_binary(bad_nsp, err));
    NXDEV_TEST_ASSERT(err.find("PFS0") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_nsp_packaging_mock_tools() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_nsp_mock_test";
    fs::create_directories(temp_dir / "assets");
    fs::create_directories(temp_dir / "romfs");
    fs::create_directories(temp_dir / "tools");

    std::string elf_path = (temp_dir / "app.elf").string();
    std::string icon_path = (temp_dir / "assets" / "icon.jpg").string();
    std::string keys_path = (temp_dir / "prod.keys").string();

    create_dummy_elf_aarch64(elf_path);
    create_dummy_jpeg(icon_path);
    { std::ofstream out(keys_path); out << "mock keys data"; }
    { std::ofstream out(temp_dir / "romfs" / "file.txt"); out << "Hello RomFS"; }

    // Mock elf2nso
    std::string mock_elf2nso = (temp_dir / "tools" / "elf2nso").string();
    {
        std::ofstream out(mock_elf2nso);
        out << "#!/bin/sh\n"
            << "touch \"$2\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_elf2nso, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Mock npdmtool
    std::string mock_npdmtool = (temp_dir / "tools" / "npdmtool").string();
    {
        std::ofstream out(mock_npdmtool);
        out << "#!/bin/sh\n"
            << "touch \"$2\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_npdmtool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Mock nacptool
    std::string mock_nacptool = (temp_dir / "tools" / "nacptool").string();
    {
        std::ofstream out(mock_nacptool);
        out << "#!/bin/sh\n"
            << "for arg in \"$@\"; do\n"
            << "  case $arg in\n"
            << "    *.nacp) OUT=$arg ;;\n"
            << "  esac\n"
            << "done\n"
            << "echo \"mock nacp metadata\" > \"$OUT\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_nacptool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Mock hacbrewpack
    std::string mock_hacbrewpack = (temp_dir / "tools" / "hacbrewpack").string();
    {
        std::ofstream out(mock_hacbrewpack);
        out << "#!/bin/sh\n"
            << "OUTDIR=\"\"\n"
            << "TID=\"\"\n"
            << "for arg in \"$@\"; do\n"
            << "  case $arg in\n"
            << "    --outdir=*) OUTDIR=\"${arg#*=}\" ;;\n"
            << "    --titleid=*) TID=\"${arg#*=}\" ;;\n"
            << "  esac\n"
            << "done\n"
            << "mkdir -p \"$OUTDIR\"\n"
            << "NSPFILE=\"$OUTDIR/${TID}.nsp\"\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(512); b[0:4]=b\"PFS0\"; b[4]=1; f.write(b); f.close()' \"$NSPFILE\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_hacbrewpack, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    // Setup request
    nxdev::pack::PackageRequest req;
    req.manifest.application().name = "Mock NSP App";
    req.manifest.application().author = "Mock Dev";
    req.manifest.application().version = "1.0.0";
    req.manifest.application().title_id = "0100000000000088";
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "assets/icon.jpg";
    req.manifest.assets().romfs.enabled = true;
    req.manifest.assets().romfs.raw_path = "romfs";
    req.project_root = temp_dir.string();
    req.input_elf_path = elf_path;
    req.keys_path = keys_path;
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NSP;
    req.elf2nso_path_override = mock_elf2nso;
    req.npdmtool_path_override = mock_npdmtool;
    req.nacptool_path_override = mock_nacptool;
    req.hacbrewpack_path_override = mock_hacbrewpack;

    std::vector<nxdev::pack::PackageStage> recorded_stages;
    auto progress_cb = [&](nxdev::pack::PackageStage stage, std::string_view) {
        recorded_stages.push_back(stage);
    };

    nxdev::pack::PackManager manager;
    auto res = manager.pack(req, nullptr, progress_cb);

    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.format == nxdev::pack::PackageFormat::NSP);
    NXDEV_TEST_ASSERT(res.file_size_bytes == 512);
    NXDEV_TEST_ASSERT(res.title_id == "0100000000000088");
    NXDEV_TEST_ASSERT(fs::exists(res.output_file));
    NXDEV_TEST_ASSERT(res.output_file.find("mock-nsp-app.nsp") != std::string::npos);

    // Check JSON serialization
    std::string json = res.to_json();
    NXDEV_TEST_ASSERT(json.find("\"status\": \"success\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"format\": \"nsp\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"titleId\": \"0100000000000088\"") != std::string::npos);
    NXDEV_TEST_ASSERT(json.find("\"backend\"") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_nsp_error_conditions() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_nsp_err_test";
    fs::create_directories(temp_dir);

    std::string elf_path = (temp_dir / "app.elf").string();
    create_dummy_elf_aarch64(elf_path);

    std::string keys_path = (temp_dir / "prod.keys").string();
    { std::ofstream out(keys_path); out << "dummy keys"; }

    nxdev::pack::PackageRequest req;
    req.project_root = temp_dir.string();
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NSP;
    req.input_elf_path = elf_path;
    req.keys_path = keys_path;
    req.elf2nso_path_override = "/bin/true";
    req.npdmtool_path_override = "/bin/true";
    req.nacptool_path_override = "/bin/true";
    req.hacbrewpack_path_override = "/bin/true";

    nxdev::pack::PackManager manager;

    // 1. Missing Title ID
    req.manifest.application().title_id.reset();
    auto res_no_tid = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_no_tid.success);
    NXDEV_TEST_ASSERT(res_no_tid.error_code == nxdev::pack::PackErrorCode::MissingTitleId);

    // 2. Invalid Title ID (invalid hex or wrong length)
    req.manifest.application().title_id = "010000000000008Z";
    auto res_bad_tid = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_bad_tid.success);
    NXDEV_TEST_ASSERT(res_bad_tid.error_code == nxdev::pack::PackErrorCode::InvalidTitleId);

    req.manifest.application().title_id = "0100000000000088";

    // 3. Missing keys file
    req.keys_path = (temp_dir / "nonexistent.keys").string();
    auto res_no_keys = manager.pack(req);
    NXDEV_TEST_ASSERT(!res_no_keys.success);
    NXDEV_TEST_ASSERT(res_no_keys.error_code == nxdev::pack::PackErrorCode::KeyFileMissing);

    fs::remove_all(temp_dir);
}

int main() {
    std::cout << "[Test] Running NXDevPack Tests...\n";

    test_sanitize_filename();
    std::cout << "  ✓ test_sanitize_filename\n";

    test_jpeg_validation();
    std::cout << "  ✓ test_jpeg_validation\n";

    test_elf_validation();
    std::cout << "  ✓ test_elf_validation\n";

    test_nro_validation();
    std::cout << "  ✓ test_nro_validation\n";

    test_nro_packaging_mock_tools();
    std::cout << "  ✓ test_nro_packaging_mock_tools\n";

    test_nro_pack_error_conditions();
    std::cout << "  ✓ test_nro_pack_error_conditions\n";

    test_npdm_generation();
    std::cout << "  ✓ test_npdm_generation\n";

    test_key_resolution();
    std::cout << "  ✓ test_key_resolution\n";

    test_pfs0_validation();
    std::cout << "  ✓ test_pfs0_validation\n";

    test_nsp_packaging_mock_tools();
    std::cout << "  ✓ test_nsp_packaging_mock_tools\n";

    test_nsp_error_conditions();
    std::cout << "  ✓ test_nsp_error_conditions\n";

    std::cout << "[Test] All NXDevPack tests passed successfully.\n";
    return 0;
}

