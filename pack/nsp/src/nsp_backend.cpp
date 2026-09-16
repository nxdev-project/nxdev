#include <nxdev/pack/nsp_backend.hpp>
#include <nxdev/pack/nro_backend.hpp>
#include <nxdev/pack/process.hpp>
#include <nxdev/pack/romfs_stager.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <unordered_set>
#include <chrono>

namespace fs = std::filesystem;

namespace nxdev::pack {

namespace sha256_impl {
    inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    inline uint32_t gam0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    inline uint32_t gam1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    inline std::string hash_bytes(const uint8_t* data, size_t length) {
        uint32_t H[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        std::vector<uint8_t> msg(data, data + length);
        uint64_t bit_len = static_cast<uint64_t>(length) * 8;
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) {
            msg.push_back(0x00);
        }
        for (int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xFF));
        }
        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t W[64];
            for (int t = 0; t < 16; ++t) {
                W[t] = (static_cast<uint32_t>(msg[chunk + t * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 2]) << 8) |
                       (static_cast<uint32_t>(msg[chunk + t * 4 + 3]));
            }
            for (int t = 16; t < 64; ++t) {
                W[t] = gam1(W[t - 2]) + W[t - 7] + gam0(W[t - 15]) + W[t - 16];
            }
            uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
            for (int t = 0; t < 64; ++t) {
                uint32_t T1 = h + sig1(e) + ch(e, f, g) + K[t] + W[t];
                uint32_t T2 = sig0(a) + maj(a, b, c);
                h = g; g = f; f = e; e = d + T1;
                d = c; c = b; b = a; a = T1 + T2;
            }
            H[0] += a; H[1] += b; H[2] += c; H[3] += d;
            H[4] += e; H[5] += f; H[6] += g; H[7] += h;
        }
        std::ostringstream ss;
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << std::setfill('0') << std::setw(8) << H[i];
        }
        return ss.str();
    }
}

std::string HacBrewPackAdapter::compute_file_sha256(const std::string& path) {
    if (!fs::exists(path)) return "missing";
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return "inaccessible";
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return sha256_impl::hash_bytes(buffer.data(), buffer.size());
}

std::vector<std::string> HacBrewPackAdapter::discover_nsp_files(const std::string& dir_path) {
    std::vector<std::string> results;
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return results;
    }
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".nsp") {
            results.push_back(entry.path().string());
        }
    }
    std::sort(results.begin(), results.end());
    return results;
}

static void safe_clean_dir(const fs::path& dir, const fs::path& allowed_parent) {
    std::error_code ec;
    if (!fs::exists(dir)) return;
    std::string dir_str = fs::weakly_canonical(dir, ec).string();
    std::string parent_str = fs::weakly_canonical(allowed_parent, ec).string();
    if (dir_str.rfind(parent_str, 0) == 0 && dir_str.length() > parent_str.length()) {
        fs::remove_all(dir, ec);
    }
}

std::string HacBrewPackAdapter::sanitize_filename(std::string_view name) {
    return NroPackBackend::sanitize_filename(name);
}

bool HacBrewPackAdapter::validate_nso_binary(const std::string& nso_path, std::string& error_msg) {
    if (!fs::exists(nso_path)) {
        error_msg = "NSO executable binary does not exist at: " + nso_path;
        return false;
    }
    std::error_code ec;
    auto sz = fs::file_size(nso_path, ec);
    if (ec || sz < 0x100) {
        error_msg = "NSO binary is invalid or too small (" + std::to_string(sz) + " bytes)";
        return false;
    }
    std::ifstream in(nso_path, std::ios::binary);
    if (!in.is_open()) {
        error_msg = "Unable to open NSO binary for reading: " + nso_path;
        return false;
    }
    char magic[4] = {0};
    in.read(magic, 4);
    if (magic[0] != 'N' || magic[1] != 'S' || magic[2] != 'O' || magic[3] != '0') {
        error_msg = "Invalid NSO executable: missing 'NSO0' header magic";
        return false;
    }
    return true;
}

bool HacBrewPackAdapter::validate_npdm_binary(const std::string& npdm_path, std::string& error_msg) {
    if (!fs::exists(npdm_path)) {
        error_msg = "NPDM metadata binary does not exist at: " + npdm_path;
        return false;
    }
    std::error_code ec;
    auto sz = fs::file_size(npdm_path, ec);
    if (ec || sz < 0x200) {
        error_msg = "NPDM binary is invalid or too small (" + std::to_string(sz) + " bytes)";
        return false;
    }
    std::ifstream in(npdm_path, std::ios::binary);
    if (!in.is_open()) {
        error_msg = "Unable to open NPDM binary for reading: " + npdm_path;
        return false;
    }
    char magic[4] = {0};
    in.read(magic, 4);
    if (magic[0] != 'M' || magic[1] != 'E' || magic[2] != 'T' || magic[3] != 'A') {
        error_msg = "Invalid NPDM binary: missing 'META' header magic";
        return false;
    }
    return true;
}

bool HacBrewPackAdapter::validate_romfs_directory(
    const std::string& romfs_path,
    const std::string& project_root,
    std::string& error_msg
) {
    if (!fs::exists(romfs_path) || !fs::is_directory(romfs_path)) {
        error_msg = "Configured RomFS directory does not exist or is not a directory: " + romfs_path;
        return false;
    }

    std::error_code ec;
    fs::path canon_romfs = fs::canonical(romfs_path, ec);
    if (ec) {
        error_msg = "Unable to resolve canonical path for RomFS directory: " + romfs_path;
        return false;
    }

    // Disallow dangerous root paths
    std::string romfs_str = canon_romfs.string();
    if (romfs_str == "/" || romfs_str == "/root") {
        error_msg = "RomFS source directory cannot point to filesystem root (" + romfs_str + ").";
        return false;
    }

    if (const char* home = std::getenv("HOME")) {
        fs::path canon_home = fs::weakly_canonical(home, ec);
        if (!ec && canon_romfs == canon_home) {
            error_msg = "RomFS source directory cannot point to user home directory (" + romfs_str + ").";
            return false;
        }
    }

    if (!project_root.empty()) {
        fs::path canon_proj = fs::weakly_canonical(project_root, ec);
        if (!ec) {
            if (canon_romfs == canon_proj) {
                error_msg = "RomFS source directory cannot be identical to project root (" + romfs_str +
                            ") because this would cause recursive inclusion of build and staging artifacts.";
                return false;
            }
            fs::path nxdev_dir = canon_proj / ".nxdev";
            std::string nxdev_str = nxdev_dir.string();
            if (romfs_str == nxdev_str || (romfs_str.rfind(nxdev_str + "/", 0) == 0)) {
                error_msg = "RomFS source directory cannot lie inside the .nxdev directory (" + romfs_str + ").";
                return false;
            }
        }
    }

    // Traverse directory and verify symlink loop / boundary safety
    std::unordered_set<std::string> visited_paths;
    visited_paths.insert(romfs_str);

    try {
        for (const auto& entry : fs::recursive_directory_iterator(canon_romfs, fs::directory_options::none, ec)) {
            if (ec) {
                error_msg = "Error accessing RomFS entry during recursion scan: " + ec.message();
                return false;
            }

            if (entry.is_symlink()) {
                fs::path target = fs::canonical(entry.path(), ec);
                if (ec) {
                    error_msg = "Broken or inaccessible symlink in RomFS directory: " + entry.path().string();
                    return false;
                }
                std::string target_str = target.string();
                if (target_str.rfind(romfs_str, 0) != 0) {
                    error_msg = "Unsafe symlink in RomFS escapes RomFS root: " + entry.path().string() + " -> " + target_str;
                    return false;
                }
                if (visited_paths.count(target_str)) {
                    error_msg = "Cyclic symlink loop detected in RomFS: " + entry.path().string() + " points back to " + target_str;
                    return false;
                }
                visited_paths.insert(target_str);
            }
        }
    } catch (const std::exception& e) {
        error_msg = std::string("Exception during RomFS safety scan: ") + e.what();
        return false;
    }

    return true;
}

bool HacBrewPackAdapter::validate_pfs0_binary(const std::string& nsp_path, std::string& error_msg) {
    if (!fs::exists(nsp_path)) {
        error_msg = "NSP file does not exist at path: " + nsp_path;
        return false;
    }
    std::error_code ec;
    auto size = fs::file_size(nsp_path, ec);
    if (ec || size < 16) {
        error_msg = "NSP file is too small or inaccessible (size: " + std::to_string(size) + " bytes)";
        return false;
    }

    std::ifstream in(nsp_path, std::ios::binary);
    if (!in.is_open()) {
        error_msg = "Unable to open NSP binary for reading: " + nsp_path;
        return false;
    }

    char magic[4] = {0};
    in.read(magic, 4);
    if (magic[0] != 'P' || magic[1] != 'F' || magic[2] != 'S' || magic[3] != '0') {
        error_msg = "Invalid NSP binary: missing 'PFS0' container magic header";
        return false;
    }

    uint32_t num_files = 0;
    in.read(reinterpret_cast<char*>(&num_files), sizeof(uint32_t));
    if (num_files == 0) {
        error_msg = "Invalid NSP binary: PFS0 container has zero files";
        return false;
    }

    return true;
}

std::string HacBrewPackAdapter::resolve_keys_file(
    const PackageRequest& request,
    [[maybe_unused]] const env::Environment* env
) {
    // 1. Explicit request override
    if (!request.keys_path.empty()) {
        if (fs::exists(request.keys_path)) {
            return request.keys_path;
        }
        return "";
    }

    // 2. NXDEV_KEYS environment variable
    if (const char* env_keys = std::getenv("NXDEV_KEYS")) {
        if (std::strlen(env_keys) > 0 && fs::exists(env_keys)) {
            return std::string(env_keys);
        }
    }

    // 3. Project-local keys (e.g. .nxdev/prod.keys or keys.dat)
    if (!request.project_root.empty()) {
        std::vector<std::string> proj_candidates = {
            (fs::path(request.project_root) / ".nxdev" / "prod.keys").string(),
            (fs::path(request.project_root) / ".nxdev" / "keys.dat").string(),
            (fs::path(request.project_root) / "prod.keys").string(),
            (fs::path(request.project_root) / "keys.dat").string()
        };
        for (const auto& c : proj_candidates) {
            if (fs::exists(c)) return c;
        }
    }

    // 4. Standard Switch default locations (~/.switch/prod.keys, ~/.switch/keys.dat)
    if (const char* home = std::getenv("HOME")) {
        std::vector<std::string> home_candidates = {
            (fs::path(home) / ".switch" / "prod.keys").string(),
            (fs::path(home) / ".switch" / "keys.dat").string(),
            (fs::path(home) / ".switch" / "keys.ini").string()
        };
        for (const auto& c : home_candidates) {
            if (fs::exists(c)) return c;
        }
    }

    return "";
}

bool HacBrewPackAdapter::generate_npdm_json(
    const manifest::Manifest& manifest,
    std::string& out_json,
    std::string& error_msg
) {
    const auto& app = manifest.application();
    const auto& npdm = manifest.npdm();

    std::string title_id = app.title_id.value_or("0100000000000001");
    std::string app_name = app.name.empty() ? "Application" : app.name;

    std::vector<std::pair<std::string, bool>> services;
    auto add_service = [&](const std::string& srv, bool is_host = false) {
        if (srv.length() < 1 || srv.length() > 8) return;
        for (const auto& existing : services) {
            if (existing.first == srv) return;
        }
        services.push_back({srv, is_host});
    };

    // Baseline standard services
    add_service("sm:");
    add_service("bsds:u");
    add_service("acc:u");
    add_service("set");
    add_service("hid");
    add_service("fsp-srv");
    add_service("nvnflinger");
    add_service("vi:u");
    add_service("appletOE");
    add_service("audren:u");
    add_service("audin:u");
    add_service("audout:u");
    add_service("time:u");
    add_service("psm");
    add_service("pl:u");

    if (npdm.preset == manifest::NpdmPreset::Network || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("bsds:s");
        add_service("bsdcfg");
        add_service("sfdnsres");
        add_service("nifm:u");
        add_service("ssl:u");
    }

    if (npdm.preset == manifest::NpdmPreset::Filesystem || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("fsp-pr");
        add_service("fsp-ldr");
    }

    if (npdm.preset == manifest::NpdmPreset::Multimedia || npdm.preset == manifest::NpdmPreset::Advanced) {
        add_service("hwopus");
        add_service("caps:u");
        add_service("caps:a");
    }

    for (const auto& s : npdm.services) {
        if (s.length() < 1 || s.length() > 8) {
            error_msg = "Invalid service name '" + s + "': service names must be between 1 and 8 characters.";
            return false;
        }
        add_service(s);
    }

    int priority = npdm.main_thread.priority;
    if (priority < 0 || priority > 63) {
        error_msg = "Invalid main thread priority " + std::to_string(priority) + ": must be between 0 and 63.";
        return false;
    }

    int core = npdm.main_thread.core;
    if (core < 0 || core > 3) {
        error_msg = "Invalid main thread CPU core " + std::to_string(core) + ": must be between 0 and 3.";
        return false;
    }

    uint32_t stack_size = npdm.main_thread.stack_size;
    if (stack_size < 0x1000) {
        error_msg = "Invalid main thread stack size " + std::to_string(stack_size) + ": minimum 4096 bytes required.";
        return false;
    }

    std::string hex_tid = title_id;
    if (hex_tid.rfind("0x", 0) != 0 && hex_tid.rfind("0X", 0) != 0) {
        hex_tid = "0x" + hex_tid;
    }

    std::ostringstream stack_hex;
    stack_hex << "0x" << std::setfill('0') << std::setw(8) << std::hex << stack_size;

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << app_name << "\",\n";
    ss << "  \"title_id\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id_range_min\": \"" << hex_tid << "\",\n";
    ss << "  \"program_id_range_max\": \"" << hex_tid << "\",\n";
    ss << "  \"main_thread_stack_size\": \"" << stack_hex.str() << "\",\n";
    ss << "  \"main_thread_priority\": " << priority << ",\n";
    ss << "  \"default_cpu_id\": " << core << ",\n";
    ss << "  \"process_category\": 0,\n";
    ss << "  \"is_64_bit\": true,\n";
    ss << "  \"address_space_type\": 1,\n";
    ss << "  \"system_resource_size\": \"0x00000000\",\n";
    ss << "  \"version\": \"0x00010000\",\n";
    ss << "  \"optimize_memory_allocation\": false,\n";
    ss << "  \"disable_device_address_space_merge\": false,\n";
    ss << "  \"enable_alias_region_extra_size\": false,\n";
    ss << "  \"prevent_code_reads\": false,\n";
    ss << "  \"signature_key_generation\": 0,\n";
    ss << "  \"is_retail\": false,\n";
    ss << "  \"pool_partition\": 0,\n";
    ss << "  \"filesystem_access\": {\n";
    ss << "    \"permissions\": \"0x0000000000000000\"\n";
    ss << "  },\n";
    ss << "  \"service_access\": [\n";
    ss << "    \"*\"\n";
    ss << "  ],\n";
    ss << "  \"service_host\": [],\n";
    ss << "  \"services\": {\n";
    for (size_t i = 0; i < services.size(); ++i) {
        ss << "    \"" << services[i].first << "\": " << (services[i].second ? "true" : "false");
        if (i + 1 < services.size()) ss << ",";
        ss << "\n";
    }
    ss << "  },\n";
    ss << "  \"kernel_capabilities\": [\n";
    ss << "    {\n";
    ss << "      \"type\": \"kernel_flags\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"highest_thread_priority\": 63,\n";
    ss << "        \"lowest_thread_priority\": 24,\n";
    ss << "        \"lowest_cpu_id\": 0,\n";
    ss << "        \"highest_cpu_id\": 3\n";
    ss << "      }\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"syscalls\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"svcSetHeapSize\": 1,\n";
    ss << "        \"svcSetMemoryAttribute\": 3,\n";
    ss << "        \"svcMapMemory\": 4,\n";
    ss << "        \"svcUnmapMemory\": 5,\n";
    ss << "        \"svcQueryMemory\": 6,\n";
    ss << "        \"svcExitProcess\": 7,\n";
    ss << "        \"svcCreateThread\": 8,\n";
    ss << "        \"svcStartThread\": 9,\n";
    ss << "        \"svcExitThread\": 10,\n";
    ss << "        \"svcSleepThread\": 11,\n";
    ss << "        \"svcGetThreadPriority\": 12,\n";
    ss << "        \"svcSetThreadPriority\": 13,\n";
    ss << "        \"svcGetThreadCoreMask\": 14,\n";
    ss << "        \"svcSetThreadCoreMask\": 15,\n";
    ss << "        \"svcGetCurrentProcessorNumber\": 16,\n";
    ss << "        \"svcSignalEvent\": 17,\n";
    ss << "        \"svcClearEvent\": 18,\n";
    ss << "        \"svcCreateInterruptEvent\": 30,\n";
    ss << "        \"svcMapSharedMemory\": 19,\n";
    ss << "        \"svcUnmapSharedMemory\": 20,\n";
    ss << "        \"svcCreateTransferMemory\": 21,\n";
    ss << "        \"svcCloseHandle\": 22,\n";
    ss << "        \"svcResetSignal\": 23,\n";
    ss << "        \"svcWaitSynchronization\": 24,\n";
    ss << "        \"svcCancelSynchronization\": 25,\n";
    ss << "        \"svcArbitrateLock\": 26,\n";
    ss << "        \"svcArbitrateUnlock\": 27,\n";
    ss << "        \"svcWaitProcessWideKeyAtomic\": 28,\n";
    ss << "        \"svcSignalProcessWideKey\": 29,\n";
    ss << "        \"svcGetSystemTick\": 31,\n";
    ss << "        \"svcConnectToNamedPort\": 32,\n";
    ss << "        \"svcSendSyncRequestLight\": 33,\n";
    ss << "        \"svcSendSyncRequest\": 34,\n";
    ss << "        \"svcSendSyncRequestWithUserBuffer\": 35,\n";
    ss << "        \"svcSendAsyncRequestWithUserBuffer\": 36,\n";
    ss << "        \"svcGetProcessId\": 37,\n";
    ss << "        \"svcGetThreadId\": 38,\n";
    ss << "        \"svcBreak\": 39,\n";
    ss << "        \"svcOutputDebugString\": 40,\n";
    ss << "        \"svcReturnFromException\": 41,\n";
    ss << "        \"svcGetInfo\": 43,\n";
    ss << "        \"svcWaitForAddress\": 52,\n";
    ss << "        \"svcSignalToAddress\": 53,\n";
    ss << "        \"svcSynchronizePreemptionState\": 90\n";
    ss << "      }\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"min_kernel_version\",\n";
    ss << "      \"value\": \"0x0030\"\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"handle_table_size\",\n";
    ss << "      \"value\": 1023\n";
    ss << "    },\n";
    ss << "    {\n";
    ss << "      \"type\": \"debug_flags\",\n";
    ss << "      \"value\": {\n";
    ss << "        \"allow_debug\": true,\n";
    ss << "        \"force_debug\": false,\n";
    ss << "        \"force_debug_prod\": false\n";
    ss << "      }\n";
    ss << "    }\n";
    ss << "  ]\n";
    ss << "}\n";

    out_json = ss.str();
    return true;
}

PackageResult NspPackBackend::pack(
    const PackageRequest& request,
    const env::Environment* env,
    ProgressCallback progress
) {
    PackageResult res;
    res.format = PackageFormat::NSP;
    res.profile = request.profile;
    res.backend_name = HacBrewPackAdapter::DEFAULT_BACKEND_NAME;
    res.backend_version = HacBrewPackAdapter::PINNED_BACKEND_REVISION;

    auto notify = [&](PackageStage stage, std::string_view msg) {
        res.log_messages.push_back("[" + package_stage_to_string(stage) + "] " + std::string(msg));
        if (progress) {
            progress(stage, msg);
        }
    };

    // -------------------------------------------------------------------------
    // Stage 1: Validation & Environment Preflight
    // -------------------------------------------------------------------------
    notify(PackageStage::Preflight, "Validating NSP identity, Title ID, and environment");

    bool is_wsl = ProcessExecutor::is_wsl_environment();
    if (is_wsl) {
        notify(PackageStage::Preflight, "Detected environment: WSL (Windows Subsystem for Linux) - executing native Linux backend");
    }

    size_t available_mem = ProcessExecutor::get_available_memory_bytes();
    if (available_mem > 0 && available_mem < 64 * 1024 * 1024) {
        notify(PackageStage::Preflight, "Warning: Host available memory is low (" + std::to_string(available_mem / (1024 * 1024)) + " MiB)");
    }

    // 1.1 Title ID requirement
    const auto& app = request.manifest.application();
    if (!app.title_id.has_value() || app.title_id->empty()) {
        res.success = false;
        res.error_code = PackErrorCode::MissingTitleId;
        res.error_message = "NSP packaging requires application.titleId configured in nxapp.yaml (e.g. titleId: \"0100000000000088\").";
        return res;
    }
    std::string raw_tid = app.title_id.value();
    if (raw_tid.length() != 16) {
        res.success = false;
        res.error_code = PackErrorCode::InvalidTitleId;
        res.error_message = "Invalid Title ID '" + raw_tid + "': Title ID must be exactly 16 hexadecimal characters.";
        return res;
    }
    for (char c : raw_tid) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidTitleId;
            res.error_message = "Invalid Title ID '" + raw_tid + "': contains non-hexadecimal character '" + std::string(1, c) + "'.";
            return res;
        }
    }
    res.title_id = raw_tid;

    // 1.2 Switch keys requirement
    std::string keys_file = HacBrewPackAdapter::resolve_keys_file(request, env);
    if (keys_file.empty()) {
        if (request.dry_run) {
            notify(PackageStage::Preflight, "Note: No Switch key file found. Real packaging will require prod.keys.");
            keys_file = "<prod.keys>";
        } else {
            res.success = false;
            res.error_code = PackErrorCode::KeyFileMissing;
            res.error_message = "NSP packaging requires a user-provided Switch key file (prod.keys).\n"
                                "Please configure a key file via:\n"
                                "  - CLI option: nxdev pack nsp --keys <path/to/prod.keys>\n"
                                "  - Environment variable: export NXDEV_KEYS=<path/to/prod.keys>\n"
                                "  - Placing prod.keys in ~/.switch/prod.keys";
            return res;
        }
    }

    // 1.3 Validate input ELF binary
    if (!request.dry_run) {
        std::string elf_err;
        if (!NroPackBackend::validate_elf_binary(request.input_elf_path, elf_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidInputBinary;
            res.error_message = elf_err;
            return res;
        }
    }
    res.input_elf_path = request.input_elf_path;

    // -------------------------------------------------------------------------
    // Stage 2: Tool Discovery (Strictly Native on Linux/WSL)
    // -------------------------------------------------------------------------
    std::string elf2nso_bin;
    std::string npdmtool_bin;
    std::string nacptool_bin;
    std::string hacbrewpack_bin;

    auto find_tool = [&](const std::string& override_path, const std::string& name) -> std::string {
        if (!override_path.empty()) {
            if (fs::exists(override_path)) {
                if (is_wsl && (override_path.rfind(".exe") != std::string::npos || override_path.rfind("/mnt/c", 0) == 0)) {
                    notify(PackageStage::Preflight, "Warning: Windows executable override detected in WSL; prefer native Linux backend.");
                }
                return override_path;
            }
        }
        try {
            if (fs::exists("/proc/self/exe")) {
                auto self_dir = fs::canonical("/proc/self/exe").parent_path();
                if (fs::exists(self_dir / name)) {
                    return (self_dir / name).string();
                }
                if (fs::exists(self_dir / ".." / "libexec" / "nxdev" / name)) {
                    return (self_dir / ".." / "libexec" / "nxdev" / name).string();
                }
            }
        } catch (...) {}

        if (!request.project_root.empty()) {
            std::vector<std::string> local_paths = {
                (fs::path(request.project_root) / "build" / "bin" / name).string(),
                (fs::path(request.project_root) / ".nxdev" / "bin" / name).string(),
                (fs::path(request.project_root) / ".." / "build" / "bin" / name).string(),
                (fs::path(request.project_root) / ".." / ".." / "build" / "bin" / name).string()
            };
            for (const auto& lp : local_paths) {
                if (fs::exists(lp)) return lp;
            }
        }

        if (const char* dkp = std::getenv("DEVKITPRO")) {
            std::string p = (fs::path(dkp) / "tools" / "bin" / name).string();
            if (fs::exists(p)) return p;
        }
        std::string std_p = (fs::path("/opt/devkitpro/tools/bin") / name).string();
        if (fs::exists(std_p)) return std_p;

        return name;
    };

    elf2nso_bin = find_tool(request.elf2nso_path_override, "elf2nso");
    npdmtool_bin = find_tool(request.npdmtool_path_override, "npdmtool");
    nacptool_bin = find_tool(request.nacptool_path_override, "nacptool");
    hacbrewpack_bin = find_tool(request.hacbrewpack_path_override, "hacbrewpack");

    if (!request.dry_run) {
        bool found_hbp = fs::exists(hacbrewpack_bin);
        if (!found_hbp) {
            std::string cmd = "which " + hacbrewpack_bin + " >/dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) {
                found_hbp = true;
            }
        }
        if (!found_hbp) {
            res.success = false;
            res.error_code = PackErrorCode::ToolNotFound;
            res.error_message = "NXDev's NSP backend is missing. Normal installations include a prebuilt hacBrewPack backend. Repair/reinstall NXDevSDK or explicitly run the backend build command ('nxdev sdk build-hacbrewpack').";
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 3: Staging Workspace Creation
    // -------------------------------------------------------------------------
    std::string project_root = request.project_root.empty() ? "." : request.project_root;
    fs::path nsp_pkg_root = fs::path(project_root) / ".nxdev" / "package" / "nsp" / request.profile;
    fs::path staging_dir = nsp_pkg_root / "staging";
    fs::path exefs_dir = staging_dir / "exefs";
    fs::path control_dir = staging_dir / "control";
    fs::path gen_dir = nsp_pkg_root / "generated";

    fs::path backend_dir = nsp_pkg_root / "backend";
    fs::path backend_temp_dir = backend_dir / "temp";
    fs::path backend_nca_dir = backend_dir / "nca";
    fs::path backend_nsp_dir = backend_dir / "nsp";
    fs::path backend_backup_dir = backend_dir / "backup";
    fs::path backend_logs_dir = nsp_pkg_root / "logs";

    if (!request.dry_run) {
        fs::create_directories(exefs_dir);
        fs::create_directories(control_dir);
        fs::create_directories(gen_dir);
        fs::create_directories(backend_logs_dir);

        // Safe clean of stale backend directories from prior runs
        safe_clean_dir(backend_temp_dir, backend_dir);
        safe_clean_dir(backend_nca_dir, backend_dir);
        safe_clean_dir(backend_nsp_dir, backend_dir);
        safe_clean_dir(backend_backup_dir, backend_dir);

        fs::create_directories(backend_temp_dir);
        fs::create_directories(backend_nca_dir);
        fs::create_directories(backend_nsp_dir);
        fs::create_directories(backend_backup_dir);
    }

    // -------------------------------------------------------------------------
    // Stage 4: Prepare ExeFS (NSO binary & NPDM)
    // -------------------------------------------------------------------------
    notify(PackageStage::PrepareExeFS, "Converting ELF binary to NSO and staging ExeFS");

    std::string nso_path = (exefs_dir / "main").string();
    res.nso_file = nso_path;

    if (!request.dry_run) {
        auto nso_res = ProcessExecutor::execute(elf2nso_bin, {request.input_elf_path, nso_path}, 30000);
        if (!nso_res.success || nso_res.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NsoConversionFailed;
            res.exit_code = nso_res.exit_code;
            res.error_message = "Failed to convert ELF to NSO via elf2nso: " +
                                (nso_res.stderr_output.empty() ? nso_res.stdout_output : nso_res.stderr_output);
            return res;
        }

        std::string nso_err;
        if (!HacBrewPackAdapter::validate_nso_binary(nso_path, nso_err)) {
            res.success = false;
            res.error_code = PackErrorCode::NsoConversionFailed;
            res.error_message = nso_err;
            return res;
        }
    }

    notify(PackageStage::GenerateNPDM, "Generating NPDM metadata specification");
    std::string npdm_json_content;
    std::string npdm_err;
    if (!HacBrewPackAdapter::generate_npdm_json(request.manifest, npdm_json_content, npdm_err)) {
        res.success = false;
        res.error_code = PackErrorCode::InvalidNpdm;
        res.error_message = npdm_err;
        return res;
    }

    std::string npdm_json_file = (gen_dir / "npdm.json").string();
    std::string npdm_bin_file = (exefs_dir / "main.npdm").string();
    res.npdm_file = npdm_bin_file;

    if (!request.dry_run) {
        {
            std::ofstream jf(npdm_json_file);
            jf << npdm_json_content;
        }

        auto npdm_res = ProcessExecutor::execute(npdmtool_bin, {npdm_json_file, npdm_bin_file}, 30000);
        if (!npdm_res.success || npdm_res.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NpdmGenerationFailed;
            res.exit_code = npdm_res.exit_code;
            res.error_message = "Failed to generate NPDM binary via npdmtool: " +
                                (npdm_res.stderr_output.empty() ? npdm_res.stdout_output : npdm_res.stderr_output);
            return res;
        }

        std::string val_npdm_err;
        if (!HacBrewPackAdapter::validate_npdm_binary(npdm_bin_file, val_npdm_err)) {
            res.success = false;
            res.error_code = PackErrorCode::NpdmGenerationFailed;
            res.error_message = val_npdm_err;
            return res;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 5: Generate Control Metadata (NACP & Icon)
    // -------------------------------------------------------------------------
    notify(PackageStage::GenerateNACP, "Generating NACP control metadata");

    std::string nacp_path = (control_dir / "control.nacp").string();
    res.nacp_file = nacp_path;

    if (!request.dry_run) {
        std::vector<std::string> nacp_args = {
            "--create",
            app.name.empty() ? "Application" : app.name,
            app.author.empty() ? "Unspecified Author" : app.author,
            app.version.empty() ? "1.0.0" : app.version,
            nacp_path,
            "--titleid=" + raw_tid
        };
        auto nacp_proc = ProcessExecutor::execute(nacptool_bin, nacp_args, 30000);
        if (!nacp_proc.success || nacp_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::NacpGenerationFailed;
            res.exit_code = nacp_proc.exit_code;
            res.error_message = "Failed to generate NACP metadata: " +
                                (nacp_proc.stderr_output.empty() ? nacp_proc.stdout_output : nacp_proc.stderr_output);
            return res;
        }
    }

    notify(PackageStage::ResolveAssets, "Resolving and validating icon and RomFS assets");

    // Icon handling
    std::string resolved_icon_path;
    const auto& icon_cfg = request.manifest.assets().icon;
    if (icon_cfg.type == manifest::IconSourceType::ProjectFile && !icon_cfg.raw_path.empty()) {
        std::string p_icon = !icon_cfg.resolved_path.empty() ?
                             icon_cfg.resolved_path :
                             (fs::path(project_root) / icon_cfg.raw_path).string();
        std::string icon_err;
        if (!request.dry_run && !NroPackBackend::is_valid_jpeg(p_icon, icon_err)) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidIcon;
            res.error_message = icon_err;
            return res;
        }
        resolved_icon_path = p_icon;
        res.icon_source = "project";
    } else {
        std::vector<std::string> icon_candidates;
        if (!request.default_icon_path_override.empty()) {
            icon_candidates.push_back(request.default_icon_path_override);
        }
        if (const char* dkp = std::getenv("DEVKITPRO")) {
            icon_candidates.push_back(std::string(dkp) + "/libnx/default_icon.jpg");
        }
        icon_candidates.push_back("/opt/devkitpro/libnx/default_icon.jpg");

        for (const auto& ic : icon_candidates) {
            if (fs::exists(ic)) {
                resolved_icon_path = ic;
                res.icon_source = "libnx_default";
                break;
            }
        }
    }

    if (!resolved_icon_path.empty()) {
        res.icon_file = resolved_icon_path;
        if (!request.dry_run) {
            std::string dest_icon = (control_dir / "icon_AmericanEnglish.dat").string();
            std::error_code ec;
            fs::copy_file(resolved_icon_path, dest_icon, fs::copy_options::overwrite_existing, ec);
        }
    }

    // RomFS handling via Shared Staging Pipeline
    bool has_romfs = false;
    std::string canonical_romfs_path;
    std::optional<std::string> user_romfs_opt;
    const auto& romfs_cfg = request.manifest.assets().romfs;
    if (romfs_cfg.enabled || !romfs_cfg.raw_path.empty()) {
        std::string p_romfs = !romfs_cfg.resolved_path.empty() ?
                              romfs_cfg.resolved_path :
                              (fs::path(project_root) / romfs_cfg.raw_path).string();
        user_romfs_opt = p_romfs;
    }

    if (!request.dry_run) {
        RomFsStageRequest stage_req;
        stage_req.project_root = project_root;
        stage_req.output_dir = (fs::path(project_root) / ".nxdev" / "build" / request.profile / "romfs").string();
        stage_req.user_romfs_path = user_romfs_opt;
        stage_req.framework_layers = RomFsStager::resolve_manifest_layers(request.manifest, project_root);
        stage_req.clean = true;
        stage_req.verbose = request.verbose;

        auto stage_res = RomFsStager::stage(stage_req);
        if (!stage_res.success) {
            res.success = false;
            res.error_code = PackErrorCode::InvalidRomFS;
            res.error_message = stage_res.error_message;
            return res;
        }

        if (stage_res.has_romfs) {
            has_romfs = true;
            canonical_romfs_path = stage_res.staged_dir;
            res.romfs_dir = canonical_romfs_path;
            res.romfs_manifest_file = stage_res.manifest_path;
            res.romfs_fingerprint = stage_res.fingerprint;
            res.romfs_files_count = stage_res.files_copied;
            res.romfs_overrides_count = stage_res.overridden_files;
            res.romfs_layers = stage_res.source_layers;
        }
    } else {
        if (user_romfs_opt.has_value() || !RomFsStager::resolve_manifest_layers(request.manifest, project_root).empty()) {
            has_romfs = true;
            canonical_romfs_path = (fs::path(project_root) / ".nxdev" / "build" / request.profile / "romfs").string();
            res.romfs_dir = canonical_romfs_path;
        }
    }

    // -------------------------------------------------------------------------
    // Stage 6: Build NSP via gayhearts/hacBrewPack
    // -------------------------------------------------------------------------
    notify(PackageStage::CreateNSP, "Invoking gayhearts/hacBrewPack backend to construct NSP package");

    std::string safe_name = HacBrewPackAdapter::sanitize_filename(app.name.empty() ? fs::path(project_root).filename().string() : app.name);
    std::string final_destination;
    if (!request.output_path.empty()) {
        final_destination = request.output_path;
    } else {
        fs::path dist_dir = fs::path(project_root) / "dist" / request.profile;
        final_destination = (dist_dir / (safe_name + ".nsp")).string();
    }

    if (!request.dry_run) {
        std::string main_staged = (exefs_dir / "main").string();
        std::string npdm_staged = (exefs_dir / "main.npdm").string();
        std::string nacp_staged = (control_dir / "control.nacp").string();
        std::string icon_staged = (control_dir / "icon_AmericanEnglish.dat").string();

        if (!fs::exists(main_staged) || fs::file_size(main_staged) == 0) {
            res.success = false;
            res.error_code = PackErrorCode::NsoConversionFailed;
            res.error_message = "ExeFS main NSO binary is missing or 0 bytes at " + main_staged;
            return res;
        }
        if (!fs::exists(npdm_staged) || fs::file_size(npdm_staged) == 0) {
            res.success = false;
            res.error_code = PackErrorCode::NpdmGenerationFailed;
            res.error_message = "ExeFS main.npdm metadata is missing or 0 bytes at " + npdm_staged;
            return res;
        }
        if (!fs::exists(nacp_staged) || fs::file_size(nacp_staged) == 0) {
            res.success = false;
            res.error_code = PackErrorCode::NacpGenerationFailed;
            res.error_message = "Control NACP metadata is missing or 0 bytes at " + nacp_staged;
            return res;
        }

        std::string hash_main = HacBrewPackAdapter::compute_file_sha256(main_staged);
        std::string hash_npdm = HacBrewPackAdapter::compute_file_sha256(npdm_staged);
        std::string hash_nacp = HacBrewPackAdapter::compute_file_sha256(nacp_staged);
        std::string hash_icon = fs::exists(icon_staged) ? HacBrewPackAdapter::compute_file_sha256(icon_staged) : "none";
        std::string hash_backend = HacBrewPackAdapter::compute_file_sha256(hacbrewpack_bin);

        size_t size_main = fs::file_size(main_staged);
        size_t size_npdm = fs::file_size(npdm_staged);
        size_t size_nacp = fs::file_size(nacp_staged);
        size_t size_icon = fs::exists(icon_staged) ? fs::file_size(icon_staged) : 0;

        std::vector<std::string> pack_args = {
            "--titleid", raw_tid,
            "--keyset", keys_file,
            "--exefsdir", exefs_dir.string(),
            "--controldir", control_dir.string(),
            "--tempdir", backend_temp_dir.string(),
            "--ncadir", backend_nca_dir.string(),
            "--nspdir", backend_nsp_dir.string(),
            "--backupdir", backend_backup_dir.string(),
            "--nologo"
        };

        if (has_romfs) {
            pack_args.push_back("--romfsdir");
            pack_args.push_back(canonical_romfs_path);
        } else {
            pack_args.push_back("--noromfs");
        }

        // Sanitized arguments for logging
        std::vector<std::string> sanitized_args;
        for (size_t i = 0; i < pack_args.size(); ++i) {
            if ((pack_args[i] == "--keyset" || pack_args[i] == "-k") && i + 1 < pack_args.size()) {
                sanitized_args.push_back(pack_args[i]);
                sanitized_args.push_back("<redacted>");
                ++i;
            } else {
                sanitized_args.push_back(pack_args[i]);
            }
        }

        fs::path log_file = backend_logs_dir / "hacbrewpack.log";
        fs::path latest_log_file = backend_logs_dir / "hacbrewpack-latest.log";
        fs::path stdout_log_file = backend_logs_dir / "hacbrewpack.stdout.log";
        fs::path stderr_log_file = backend_logs_dir / "hacbrewpack.stderr.log";

        // Write launch metadata header to log
        auto start_tp = std::chrono::system_clock::now();
        auto start_time_t = std::chrono::system_clock::to_time_t(start_tp);
        std::string start_str = std::ctime(&start_time_t);
        if (!start_str.empty() && start_str.back() == '\n') start_str.pop_back();

        std::ostringstream header_ss;
        header_ss << "================================================================================\n"
                  << "NXDev NSP Packaging Backend Execution Log\n"
                  << "================================================================================\n"
                  << "NXDev Version:       1.0.0\n"
                  << "NXDevPack Version:   1.0.0\n"
                  << "Backend Name:        " << HacBrewPackAdapter::DEFAULT_BACKEND_NAME << "\n"
                  << "Backend Upstream:    " << HacBrewPackAdapter::UPSTREAM_URL << "\n"
                  << "Backend Revision:    " << HacBrewPackAdapter::PINNED_BACKEND_REVISION << "\n"
                  << "Backend Executable:  " << hacbrewpack_bin << "\n"
                  << "Backend SHA-256:     " << hash_backend << "\n"
                  << "Working Directory:   " << backend_dir.string() << "\n"
                  << "Staging Directory:   " << staging_dir.string() << "\n"
                  << "Profile:             " << request.profile << "\n"
                  << "Title ID:            " << raw_tid << "\n"
                  << "ExeFS Path:          " << exefs_dir.string() << "\n"
                  << "Control Path:        " << control_dir.string() << "\n"
                  << "RomFS Path:          " << (has_romfs ? canonical_romfs_path : "disabled") << "\n"
                  << "Temp Path:           " << backend_temp_dir.string() << "\n"
                  << "NCA Output Path:     " << backend_nca_dir.string() << "\n"
                  << "NSP Output Path:     " << backend_nsp_dir.string() << "\n"
                  << "Keys Configured:     " << (keys_file.empty() ? "no" : "yes") << "\n"
                  << "Host Environment:    " << (is_wsl ? "Linux (WSL native)" : "Linux native") << "\n"
                  << "Available Memory:    " << (available_mem / (1024 * 1024)) << " MiB\n"
                  << "Start Time:          " << start_str << "\n\n"
                  << "--------------------------------------------------------------------------------\n"
                  << "Command Invocation\n"
                  << "--------------------------------------------------------------------------------\n"
                  << "Arguments:\n";
        for (const auto& a : sanitized_args) {
            header_ss << "  " << a << "\n";
        }
        header_ss << "\n--------------------------------------------------------------------------------\n"
                  << "Staged Input Integrity\n"
                  << "--------------------------------------------------------------------------------\n"
                  << "  main                      " << std::setw(10) << size_main << " bytes (sha256: " << hash_main << ")\n"
                  << "  main.npdm                 " << std::setw(10) << size_npdm << " bytes (sha256: " << hash_npdm << ")\n"
                  << "  control.nacp              " << std::setw(10) << size_nacp << " bytes (sha256: " << hash_nacp << ")\n";
        if (size_icon > 0) {
            header_ss << "  icon_AmericanEnglish.dat  " << std::setw(10) << size_icon << " bytes (sha256: " << hash_icon << ")\n";
        }
        header_ss << "\n--------------------------------------------------------------------------------\n"
                  << "Backend Live Output Stream\n"
                  << "--------------------------------------------------------------------------------\n";

        {
            std::ofstream init_log(log_file, std::ios::trunc);
            init_log << header_ss.str();
        }
        {
            std::ofstream init_stdout(stdout_log_file, std::ios::trunc);
            std::ofstream init_stderr(stderr_log_file, std::ios::trunc);
        }

        // Configure process limits and safety ceiling
        ProcessOptions proc_opt;
        proc_opt.timeout_ms = 120000;
        proc_opt.working_dir = backend_dir.string();
        proc_opt.log_file_path = log_file.string();
        proc_opt.stdout_log_path = stdout_log_file.string();
        proc_opt.stderr_log_path = stderr_log_file.string();
        proc_opt.max_output_tail_bytes = 65536; // 64 KiB bounded tail

        // Set memory safety ceiling (conservative: 3.5 GiB max or 80% available RAM)
        size_t safety_ceiling = (available_mem > 0) ? std::min(available_mem * 8 / 10, static_cast<size_t>(3ULL * 1024 * 1024 * 1024)) : 0;
        proc_opt.max_memory_bytes = safety_ceiling;

        auto pack_proc = ProcessExecutor::execute(hacbrewpack_bin, pack_args, proc_opt);

        auto end_tp = std::chrono::system_clock::now();
        auto end_time_t = std::chrono::system_clock::to_time_t(end_tp);
        std::string end_str = std::ctime(&end_time_t);
        if (!end_str.empty() && end_str.back() == '\n') end_str.pop_back();

        // Discover generated outputs
        auto discovered_nsps = HacBrewPackAdapter::discover_nsp_files(backend_nsp_dir.string());

        // Append execution summary to log file
        {
            std::ofstream log_append(log_file, std::ios::app);
            log_append << "\n--------------------------------------------------------------------------------\n"
                       << "Backend Execution Summary\n"
                       << "--------------------------------------------------------------------------------\n"
                       << "Exit Code:           " << pack_proc.exit_code << "\n"
                       << "Terminated by Signal:" << (pack_proc.terminated_by_signal ? (" yes (SIG " + std::to_string(pack_proc.termination_signal) + ")") : " no") << "\n"
                       << "Peak Memory (RSS):   " << (pack_proc.peak_rss_bytes / (1024 * 1024)) << " MiB\n"
                       << "Duration:            " << pack_proc.duration_ms << " ms\n"
                       << "End Time:            " << end_str << "\n"
                       << "Discovered NSPs:\n";
            if (discovered_nsps.empty()) {
                log_append << "  <none>\n";
            } else {
                for (const auto& p : discovered_nsps) {
                    std::error_code ec;
                    log_append << "  " << p << " (" << fs::file_size(p, ec) << " bytes)\n";
                }
            }
            log_append << "================================================================================\n";
        }

        // Duplicate to latest log
        std::error_code ec_log;
        fs::copy_file(log_file, latest_log_file, fs::copy_options::overwrite_existing, ec_log);

        // Structured execution info
        BackendExecutionInfo binfo;
        binfo.name = HacBrewPackAdapter::DEFAULT_BACKEND_NAME;
        binfo.revision = HacBrewPackAdapter::PINNED_BACKEND_REVISION;
        binfo.executable = hacbrewpack_bin;
        binfo.stage = "CreateNSP";
        binfo.working_dir = backend_dir.string();
        binfo.log_path = log_file.string();
        binfo.staging_dir = staging_dir.string();
        binfo.exit_code = pack_proc.exit_code;
        binfo.stdout_output = pack_proc.stdout_output;
        binfo.stderr_output = pack_proc.stderr_output;
        binfo.keys_configured = !keys_file.empty();
        res.backend_info = binfo;

        if (!pack_proc.success || pack_proc.exit_code != 0) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.exit_code = pack_proc.exit_code;

            std::string output_snippet = pack_proc.stderr_output.empty() ? pack_proc.stdout_output : pack_proc.stderr_output;
            while (!output_snippet.empty() && (output_snippet.back() == '\n' || output_snippet.back() == '\r' || output_snippet.back() == ' ')) {
                output_snippet.pop_back();
            }

            std::ostringstream err_ss;
            err_ss << "NSP packaging failed during hacBrewPack execution.\n\n"
                   << "Backend:\n"
                   << "  " << HacBrewPackAdapter::DEFAULT_BACKEND_NAME << "\n"
                   << "  revision: " << HacBrewPackAdapter::PINNED_BACKEND_REVISION << "\n\n"
                   << "Process:\n";
            if (pack_proc.terminated_by_signal) {
                err_ss << "  exit: signal SIG" << pack_proc.termination_signal << "\n";
            } else {
                err_ss << "  exit code: " << pack_proc.exit_code << "\n";
            }
            err_ss << "  peak memory: " << (pack_proc.peak_rss_bytes / (1024 * 1024)) << " MiB\n\n";

            if (pack_proc.memory_limit_exceeded) {
                err_ss << "Error: hacBrewPack exceeded the configured memory safety ceiling ("
                       << (safety_ceiling / (1024 * 1024)) << " MiB).\n\n";
            } else if (pack_proc.possible_oom_killed) {
                err_ss << "Warning: hacBrewPack was killed by SIGKILL. The process may have been terminated by the Linux/WSL OOM killer.\n\n";
            }

            err_ss << "Backend log:\n"
                   << "  " << log_file.string() << "\n\n";

            if (!output_snippet.empty()) {
                err_ss << "Last backend output:\n"
                       << "  " << output_snippet << "\n\n";
            } else {
                err_ss << "hacBrewPack exited with code " << pack_proc.exit_code << " and produced no stdout or stderr.\n\n";
            }

            err_ss << "Staging preserved:\n"
                   << "  " << backend_dir.string();

            res.error_message = err_ss.str();
            return res;
        }

        // -------------------------------------------------------------------------
        // Stage 7: Validation & Atomic Final Placement
        // -------------------------------------------------------------------------
        notify(PackageStage::Validate, "Validating NSP PFS0 container structure");

        if (discovered_nsps.empty()) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            std::ostringstream err_ss;
            err_ss << "hacBrewPack completed but no output .nsp file was found in NSP output directory ("
                   << backend_nsp_dir.string() << ").\n\n"
                   << "Backend log:\n"
                   << "  " << log_file.string() << "\n\n"
                   << "Backend workspace preserved:\n"
                   << "  " << backend_dir.string();
            res.error_message = err_ss.str();
            return res;
        }

        std::string raw_tid_lower = raw_tid;
        std::transform(raw_tid_lower.begin(), raw_tid_lower.end(), raw_tid_lower.begin(), [](unsigned char c) { return std::tolower(c); });

        std::string found_staged_nsp;
        for (const auto& p : discovered_nsps) {
            std::string fname = fs::path(p).filename().string();
            if (fname == raw_tid_lower + ".nsp" || fname == raw_tid + ".nsp" || fname == safe_name + ".nsp") {
                found_staged_nsp = p;
                break;
            }
        }
        if (found_staged_nsp.empty()) {
            found_staged_nsp = discovered_nsps[0];
        }

        std::string pfs0_err;
        if (!HacBrewPackAdapter::validate_pfs0_binary(found_staged_nsp, pfs0_err)) {
            res.success = false;
            res.error_code = PackErrorCode::ValidationFailed;
            res.error_message = "NSP PFS0 container validation failed: " + pfs0_err + "\n\nBackend log:\n  " + log_file.string();
            return res;
        }

        notify(PackageStage::Finalize, "Placing final NSP artifact at " + final_destination);
        fs::path dest_parent = fs::path(final_destination).parent_path();
        if (!dest_parent.empty()) {
            fs::create_directories(dest_parent);
        }

        // Atomic publish: copy to temporary file first, then rename
        std::string temp_publish_dest = final_destination + ".tmp." + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::error_code ec;
        fs::copy_file(found_staged_nsp, temp_publish_dest, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            res.success = false;
            res.error_code = PackErrorCode::PackagingFailed;
            res.error_message = "Failed to copy final NSP artifact to temporary destination: " + ec.message();
            return res;
        }

        fs::rename(temp_publish_dest, final_destination, ec);
        if (ec) {
            // Fallback if atomic rename across filesystems fails
            fs::copy_file(temp_publish_dest, final_destination, fs::copy_options::overwrite_existing, ec);
            fs::remove(temp_publish_dest, ec);
        }

        res.file_size_bytes = fs::file_size(final_destination, ec);

        // Workspace retention policy
        if (!request.keep_staging) {
            safe_clean_dir(backend_temp_dir, backend_dir);
            safe_clean_dir(backend_nca_dir, backend_dir);
            safe_clean_dir(backend_backup_dir, backend_dir);
        }
    } else {
        notify(PackageStage::Finalize, "[Dry-Run] Simulated placement at " + final_destination);
    }

    res.success = true;
    res.output_file = final_destination;
    return res;
}

} // namespace nxdev::pack
