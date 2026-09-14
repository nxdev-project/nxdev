#include "test_common.hpp"
#include <nxdev/pack/pack.hpp>
#include <nxdev/pack/process.hpp>
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
    buf[0x10] = 'N'; buf[0x11] = 'R'; buf[0x12] = 'O'; buf[0x13] = '0';
    out.write(buf, sizeof(buf));
}

static void create_dummy_nso(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    char buf[512] = {0};
    buf[0] = 'N'; buf[1] = 'S'; buf[2] = 'O'; buf[3] = '0';
    out.write(buf, sizeof(buf));
}

static void create_dummy_npdm(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    char buf[1024] = {0};
    buf[0] = 'M'; buf[1] = 'E'; buf[2] = 'T'; buf[3] = 'A';
    out.write(buf, sizeof(buf));
}

static void create_dummy_pfs0(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    char buf[512] = {0};
    buf[0] = 'P'; buf[1] = 'F'; buf[2] = 'S'; buf[3] = '0';
    uint32_t num_files = 1;
    std::memcpy(&buf[4], &num_files, sizeof(uint32_t));
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
    NXDEV_TEST_ASSERT(err.find("AArch64") != std::string::npos);

    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_elf_binary(bad_elf, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_elf_binary((temp_dir / "nonexistent.elf").string(), err));

    fs::remove_all(temp_dir);
}

void test_nro_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_nro";
    fs::create_directories(temp_dir);

    std::string valid_nro = (temp_dir / "app.nro").string();
    std::string bad_nro = (temp_dir / "bad.nro").string();

    create_dummy_nro(valid_nro);
    { std::ofstream out(bad_nro); out << "not an nro binary with more than thirty two bytes of text padding here"; }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::NroPackBackend::validate_nro_binary(valid_nro, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::NroPackBackend::validate_nro_binary(bad_nro, err));
    NXDEV_TEST_ASSERT(err.find("NRO0") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_nso_and_npdm_validation() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_nso_npdm";
    fs::create_directories(temp_dir);

    std::string valid_nso = (temp_dir / "main").string();
    std::string bad_nso = (temp_dir / "bad_main").string();
    std::string valid_npdm = (temp_dir / "main.npdm").string();
    std::string bad_npdm = (temp_dir / "bad_main.npdm").string();

    create_dummy_nso(valid_nso);
    { std::ofstream out(bad_nso); std::vector<char> pad(512, 'X'); out.write(pad.data(), pad.size()); }
    create_dummy_npdm(valid_npdm);
    { std::ofstream out(bad_npdm); std::vector<char> pad(1024, 'Y'); out.write(pad.data(), pad.size()); }

    std::string err;
    NXDEV_TEST_ASSERT(nxdev::pack::HacBrewPackAdapter::validate_nso_binary(valid_nso, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_nso_binary(bad_nso, err));
    NXDEV_TEST_ASSERT(err.find("NSO0") != std::string::npos);

    NXDEV_TEST_ASSERT(nxdev::pack::HacBrewPackAdapter::validate_npdm_binary(valid_npdm, err));
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_npdm_binary(bad_npdm, err));
    NXDEV_TEST_ASSERT(err.find("META") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_romfs_recursion_prevention() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_romfs_recurse";
    fs::create_directories(temp_dir / "romfs_valid" / "sub");
    fs::create_directories(temp_dir / ".nxdev" / "package");

    { std::ofstream out(temp_dir / "romfs_valid" / "sub" / "data.bin"); out << "123"; }

    std::string err;
    // 1. Valid separate RomFS directory
    NXDEV_TEST_ASSERT(nxdev::pack::HacBrewPackAdapter::validate_romfs_directory(
        (temp_dir / "romfs_valid").string(), temp_dir.string(), err));

    // 2. Reject RomFS pointing directly to project root
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_romfs_directory(
        temp_dir.string(), temp_dir.string(), err));
    NXDEV_TEST_ASSERT(err.find("identical to project root") != std::string::npos);

    // 3. Reject RomFS pointing to .nxdev
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_romfs_directory(
        (temp_dir / ".nxdev").string(), temp_dir.string(), err));
    NXDEV_TEST_ASSERT(err.find(".nxdev") != std::string::npos);

    // 4. Reject RomFS pointing to root '/'
    NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_romfs_directory(
        "/", temp_dir.string(), err));
    NXDEV_TEST_ASSERT(err.find("filesystem root") != std::string::npos);

    // 5. Reject RomFS with symlink escaping RomFS root
    fs::path romfs_escaping = temp_dir / "romfs_escape";
    fs::create_directories(romfs_escaping);
    std::error_code ec;
    fs::create_directory_symlink(temp_dir, romfs_escaping / "escape_link", ec);
    if (!ec) {
        NXDEV_TEST_ASSERT(!nxdev::pack::HacBrewPackAdapter::validate_romfs_directory(
            romfs_escaping.string(), temp_dir.string(), err));
        NXDEV_TEST_ASSERT(err.find("escapes RomFS root") != std::string::npos || err.find("symlink") != std::string::npos);
    }

    fs::remove_all(temp_dir);
}

void test_gayhearts_backend_metadata() {
    using nxdev::pack::HacBrewPackAdapter;
    NXDEV_TEST_ASSERT(std::string(HacBrewPackAdapter::DEFAULT_BACKEND_NAME) == "gayhearts/hacBrewPack");
    NXDEV_TEST_ASSERT(std::string(HacBrewPackAdapter::PINNED_BACKEND_REVISION) == "1a5f378c1b5747c603f4a50a4a97d86cc7c05fd4");
    NXDEV_TEST_ASSERT(std::string(HacBrewPackAdapter::PINNED_BACKEND_VERSION) == "3.17");
    NXDEV_TEST_ASSERT(std::string(HacBrewPackAdapter::UPSTREAM_URL) == "https://github.com/gayhearts/hacBrewPack");
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

    std::string resolved = HacBrewPackAdapter::resolve_keys_file(req);
    NXDEV_TEST_ASSERT(resolved == custom_keys);

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

void test_process_executor_huge_output_streaming() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_huge_out";
    fs::create_directories(temp_dir);

    std::string huge_script = (temp_dir / "huge_output.sh").string();
    {
        std::ofstream out(huge_script);
        out << "#!/bin/sh\n"
            << "for i in $(seq 1 5000); do\n"
            << "  echo \"Line $i: This is repeated backend output that simulates large logs...\"\n"
            << "done\n"
            << "exit 0\n";
    }
    fs::permissions(huge_script, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    std::string log_file = (temp_dir / "output.log").string();
    nxdev::pack::ProcessOptions opt;
    opt.log_file_path = log_file;
    opt.max_output_tail_bytes = 4096; // Bound in-memory tail to 4KB

    auto res = nxdev::pack::ProcessExecutor::execute(huge_script, {}, opt);
    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.exit_code == 0);

    // In-memory buffer must be strictly bounded
    NXDEV_TEST_ASSERT(res.stdout_output.size() <= 4096);
    NXDEV_TEST_ASSERT(res.stdout_output.find("Line 5000") != std::string::npos);

    // Log file on disk must contain complete full output
    NXDEV_TEST_ASSERT(fs::exists(log_file));
    auto log_sz = fs::file_size(log_file);
    NXDEV_TEST_ASSERT(log_sz > 100000); // More than 100KB on disk

    fs::remove_all(temp_dir);
}

struct MockToolPaths {
    std::string elf2nso;
    std::string npdmtool;
    std::string nacptool;
};

static MockToolPaths setup_standard_mock_tools(const fs::path& tools_dir) {
    fs::create_directories(tools_dir);
    MockToolPaths tools;
    tools.elf2nso = (tools_dir / "elf2nso").string();
    {
        std::ofstream out(tools.elf2nso);
        out << "#!/bin/sh\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(512); b[0:4]=b\"NSO0\"; f.write(b); f.close()' \"$2\"\n"
            << "exit 0\n";
    }
    fs::permissions(tools.elf2nso, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    tools.npdmtool = (tools_dir / "npdmtool").string();
    {
        std::ofstream out(tools.npdmtool);
        out << "#!/bin/sh\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(1024); b[0:4]=b\"META\"; f.write(b); f.close()' \"$2\"\n"
            << "exit 0\n";
    }
    fs::permissions(tools.npdmtool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    tools.nacptool = (tools_dir / "nacptool").string();
    {
        std::ofstream out(tools.nacptool);
        out << "#!/bin/sh\n"
            << "for arg in \"$@\"; do\n"
            << "  case $arg in\n"
            << "    *.nacp) OUT=$arg ;;\n"
            << "  esac\n"
            << "done\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(16384); b[0:4]=b\"NACP\"; f.write(b); f.close()' \"$OUT\"\n"
            << "exit 0\n";
    }
    fs::permissions(tools.nacptool, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    return tools;
}

void test_nsp_packaging_mock_tools() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_nsp_mock_test";
    fs::create_directories(temp_dir / "assets");
    fs::create_directories(temp_dir / "romfs_assets");
    auto mock_tools = setup_standard_mock_tools(temp_dir / "tools");

    std::string elf_path = (temp_dir / "app.elf").string();
    std::string icon_path = (temp_dir / "assets" / "icon.jpg").string();
    std::string keys_path = (temp_dir / "prod.keys").string();

    create_dummy_elf_aarch64(elf_path);
    create_dummy_jpeg(icon_path);
    { std::ofstream out(keys_path); out << "mock keys data"; }
    { std::ofstream out(temp_dir / "romfs_assets" / "file.txt"); out << "Hello RomFS"; }

    // Mock gayhearts/hacbrewpack backend
    std::string mock_hacbrewpack = (temp_dir / "tools" / "hacbrewpack").string();
    {
        std::ofstream out(mock_hacbrewpack);
        out << "#!/bin/sh\n"
            << "OUTDIR=\"\"\n"
            << "TID=\"\"\n"
            << "while [ $# -gt 0 ]; do\n"
            << "  case $1 in\n"
            << "    --nspdir) OUTDIR=\"$2\"; shift 2 ;;\n"
            << "    --titleid) TID=\"$2\"; shift 2 ;;\n"
            << "    *) shift ;;\n"
            << "  esac\n"
            << "done\n"
            << "mkdir -p \"$OUTDIR\"\n"
            << "NSPFILE=\"$OUTDIR/${TID}.nsp\"\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(512); b[0:4]=b\"PFS0\"; b[4]=1; f.write(b); f.close()' \"$NSPFILE\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_hacbrewpack, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    nxdev::pack::PackageRequest req;
    req.manifest.application().name = "Mock NSP App";
    req.manifest.application().author = "Mock Dev";
    req.manifest.application().version = "1.0.0";
    req.manifest.application().title_id = "0100000000000088";
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "assets/icon.jpg";
    req.manifest.assets().romfs.enabled = true;
    req.manifest.assets().romfs.raw_path = "romfs_assets";
    req.project_root = temp_dir.string();
    req.input_elf_path = elf_path;
    req.keys_path = keys_path;
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NSP;
    req.elf2nso_path_override = mock_tools.elf2nso;
    req.npdmtool_path_override = mock_tools.npdmtool;
    req.nacptool_path_override = mock_tools.nacptool;
    req.hacbrewpack_path_override = mock_hacbrewpack;

    nxdev::pack::PackManager manager;
    auto res = manager.pack(req);

    NXDEV_TEST_ASSERT(res.success);
    NXDEV_TEST_ASSERT(res.format == nxdev::pack::PackageFormat::NSP);
    NXDEV_TEST_ASSERT(res.file_size_bytes == 512);
    NXDEV_TEST_ASSERT(res.title_id == "0100000000000088");
    NXDEV_TEST_ASSERT(fs::exists(res.output_file));
    NXDEV_TEST_ASSERT(res.output_file.find("mock-nsp-app.nsp") != std::string::npos);

    // Check backend log was written
    fs::path log_path = temp_dir / ".nxdev" / "package" / "nsp" / "debug" / "logs" / "hacbrewpack.log";
    fs::path latest_log_path = temp_dir / ".nxdev" / "package" / "nsp" / "debug" / "logs" / "hacbrewpack-latest.log";
    NXDEV_TEST_ASSERT(fs::exists(log_path));
    NXDEV_TEST_ASSERT(fs::exists(latest_log_path));

    std::ifstream log_in(log_path);
    std::string log_contents((std::istreambuf_iterator<char>(log_in)), std::istreambuf_iterator<char>());
    NXDEV_TEST_ASSERT(log_contents.find("Backend Name:        gayhearts/hacBrewPack") != std::string::npos);
    NXDEV_TEST_ASSERT(log_contents.find("Backend Revision:    1a5f378c1b5747c603f4a50a4a97d86cc7c05fd4") != std::string::npos);
    NXDEV_TEST_ASSERT(log_contents.find("Exit Code:           0") != std::string::npos);
    NXDEV_TEST_ASSERT(log_contents.find("<redacted>") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_backend_error_detailed_diagnostics() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_backend_err_diag";
    fs::create_directories(temp_dir / "assets");
    auto mock_tools = setup_standard_mock_tools(temp_dir / "tools");

    std::string elf_path = (temp_dir / "app.elf").string();
    std::string icon_path = (temp_dir / "assets" / "icon.jpg").string();
    std::string keys_path = (temp_dir / "prod.keys").string();

    create_dummy_elf_aarch64(elf_path);
    create_dummy_jpeg(icon_path);
    { std::ofstream out(keys_path); out << "dummy keys"; }

    std::string mock_hbp_fail = (temp_dir / "tools" / "hacbrewpack_fail").string();
    {
        std::ofstream out(mock_hbp_fail);
        out << "#!/bin/sh\n"
            << "echo \"Error: Key area encryption key header_key missing from keyset\" >&2\n"
            << "exit 1\n";
    }
    fs::permissions(mock_hbp_fail, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    nxdev::pack::PackageRequest req;
    req.manifest.application().name = "Err App";
    req.manifest.application().title_id = "0100000000000099";
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "assets/icon.jpg";
    req.project_root = temp_dir.string();
    req.input_elf_path = elf_path;
    req.keys_path = keys_path;
    req.profile = "release";
    req.format = nxdev::pack::PackageFormat::NSP;
    req.elf2nso_path_override = mock_tools.elf2nso;
    req.npdmtool_path_override = mock_tools.npdmtool;
    req.nacptool_path_override = mock_tools.nacptool;
    req.hacbrewpack_path_override = mock_hbp_fail;

    nxdev::pack::PackManager manager;
    auto res = manager.pack(req);

    NXDEV_TEST_ASSERT(!res.success);
    NXDEV_TEST_ASSERT(res.error_code == nxdev::pack::PackErrorCode::PackagingFailed);
    NXDEV_TEST_ASSERT(res.exit_code == 1);
    NXDEV_TEST_ASSERT(res.error_message.find("gayhearts/hacBrewPack") != std::string::npos);
    NXDEV_TEST_ASSERT(res.error_message.find("header_key missing") != std::string::npos);
    NXDEV_TEST_ASSERT(res.error_message.find("Backend log:") != std::string::npos);

    fs::remove_all(temp_dir);
}

void test_clean_workspace_successive_runs() {
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_clean_ws_test";
    fs::create_directories(temp_dir / "assets");
    auto mock_tools = setup_standard_mock_tools(temp_dir / "tools");

    std::string elf_path = (temp_dir / "app.elf").string();
    std::string icon_path = (temp_dir / "assets" / "icon.jpg").string();
    std::string keys_path = (temp_dir / "prod.keys").string();

    create_dummy_elf_aarch64(elf_path);
    create_dummy_jpeg(icon_path);
    { std::ofstream out(keys_path); out << "dummy keys"; }

    fs::path backend_temp = temp_dir / ".nxdev" / "package" / "nsp" / "debug" / "backend" / "temp";
    fs::path backend_nsp = temp_dir / ".nxdev" / "package" / "nsp" / "debug" / "backend" / "nsp";
    fs::create_directories(backend_temp);
    fs::create_directories(backend_nsp);

    { std::ofstream out(backend_temp / "stale_file.bin"); out << "stale"; }
    { std::ofstream out(backend_nsp / "old_stale.nsp"); out << "stale"; }

    std::string mock_hacbrewpack = (temp_dir / "tools" / "hacbrewpack").string();
    {
        std::ofstream out(mock_hacbrewpack);
        out << "#!/bin/sh\n"
            << "OUTDIR=\"\"\n"
            << "TID=\"\"\n"
            << "while [ $# -gt 0 ]; do\n"
            << "  case $1 in\n"
            << "    --nspdir) OUTDIR=\"$2\"; shift 2 ;;\n"
            << "    --titleid) TID=\"$2\"; shift 2 ;;\n"
            << "    *) shift ;;\n"
            << "  esac\n"
            << "done\n"
            << "mkdir -p \"$OUTDIR\"\n"
            << "NSPFILE=\"$OUTDIR/${TID}.nsp\"\n"
            << "python3 -c 'import sys; f=open(sys.argv[1], \"wb\"); b=bytearray(512); b[0:4]=b\"PFS0\"; b[4]=1; f.write(b); f.close()' \"$NSPFILE\"\n"
            << "exit 0\n";
    }
    fs::permissions(mock_hacbrewpack, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, fs::perm_options::add);

    nxdev::pack::PackageRequest req;
    req.manifest.application().name = "Clean App";
    req.manifest.application().title_id = "0100000000000077";
    req.manifest.assets().icon.type = nxdev::manifest::IconSourceType::ProjectFile;
    req.manifest.assets().icon.raw_path = "assets/icon.jpg";
    req.project_root = temp_dir.string();
    req.input_elf_path = elf_path;
    req.keys_path = keys_path;
    req.profile = "debug";
    req.format = nxdev::pack::PackageFormat::NSP;
    req.elf2nso_path_override = mock_tools.elf2nso;
    req.npdmtool_path_override = mock_tools.npdmtool;
    req.nacptool_path_override = mock_tools.nacptool;
    req.hacbrewpack_path_override = mock_hacbrewpack;

    nxdev::pack::PackManager manager;
    auto res = manager.pack(req);

    NXDEV_TEST_ASSERT(res.success);
    auto discovered = nxdev::pack::HacBrewPackAdapter::discover_nsp_files(backend_nsp.string());
    for (const auto& d : discovered) {
        NXDEV_TEST_ASSERT(d.find("old_stale.nsp") == std::string::npos);
    }

    fs::remove_all(temp_dir);
}

void test_sha256_computation() {
    using nxdev::pack::HacBrewPackAdapter;
    fs::path temp_dir = fs::temp_directory_path() / "nxdev_test_sha";
    fs::create_directories(temp_dir);

    std::string empty_file = (temp_dir / "empty.txt").string();
    { std::ofstream out(empty_file); }

    std::string hash = HacBrewPackAdapter::compute_file_sha256(empty_file);
    NXDEV_TEST_ASSERT(hash == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    NXDEV_TEST_ASSERT(HacBrewPackAdapter::compute_file_sha256((temp_dir / "nonexistent.bin").string()) == "missing");

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

    test_nso_and_npdm_validation();
    std::cout << "  ✓ test_nso_and_npdm_validation\n";

    test_romfs_recursion_prevention();
    std::cout << "  ✓ test_romfs_recursion_prevention\n";

    test_gayhearts_backend_metadata();
    std::cout << "  ✓ test_gayhearts_backend_metadata\n";

    test_npdm_generation();
    std::cout << "  ✓ test_npdm_generation\n";

    test_key_resolution();
    std::cout << "  ✓ test_key_resolution\n";

    test_pfs0_validation();
    std::cout << "  ✓ test_pfs0_validation\n";

    test_sha256_computation();
    std::cout << "  ✓ test_sha256_computation\n";

    test_process_executor_huge_output_streaming();
    std::cout << "  ✓ test_process_executor_huge_output_streaming\n";

    test_nsp_packaging_mock_tools();
    std::cout << "  ✓ test_nsp_packaging_mock_tools\n";

    test_backend_error_detailed_diagnostics();
    std::cout << "  ✓ test_backend_error_detailed_diagnostics\n";

    test_clean_workspace_successive_runs();
    std::cout << "  ✓ test_clean_workspace_successive_runs\n";

    std::cout << "[Test] All NXDevPack tests passed successfully.\n";
    return 0;
}
