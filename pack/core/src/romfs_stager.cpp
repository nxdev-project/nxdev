#include <nxdev/pack/romfs_stager.hpp>
#include <nxdev/pack/pack.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace nxdev::pack {

namespace {

// Internal self-contained SHA-256 implementation
namespace sha256_internal {

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t theta0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t theta1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::string hash_bytes(const uint8_t* data, size_t length) {
    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    uint64_t total_bits = length * 8;
    size_t padded_len = ((length + 8) / 64 + 1) * 64;
    std::vector<uint8_t> msg(padded_len, 0);
    if (length > 0) {
        std::copy(data, data + length, msg.begin());
    }
    msg[length] = 0x80;
    for (int i = 0; i < 8; ++i) {
        msg[padded_len - 1 - i] = static_cast<uint8_t>((total_bits >> (i * 8)) & 0xFF);
    }

    for (size_t chunk = 0; chunk < padded_len; chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = theta1(w[i - 2]) + w[i - 7] + theta0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
            uint32_t t2 = sig0(a) + majority(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += h;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    uint32_t hash_words[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int i = 0; i < 8; ++i) {
        ss << std::setw(8) << hash_words[i];
    }
    return ss.str();
}

} // namespace sha256_internal

static std::string escape_json_str(std::string_view s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

static void safe_clean_staging_dir(const fs::path& dir, const fs::path& allowed_parent) {
    std::error_code ec;
    if (!fs::exists(dir)) return;
    std::string dir_str = fs::weakly_canonical(dir, ec).string();
    std::string parent_str = fs::weakly_canonical(allowed_parent, ec).string();
    if (dir_str.rfind(parent_str, 0) == 0 && dir_str.length() > parent_str.length()) {
        fs::remove_all(dir, ec);
    }
}

static std::string to_lower_str(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace

std::string RomFsStager::compute_file_sha256(const std::string& file_path) {
    if (!fs::exists(file_path)) return "missing";
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) return "inaccessible";
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return sha256_internal::hash_bytes(buffer.data(), buffer.size());
}

bool RomFsStager::is_safe_staging_path(
    const std::string& candidate_path,
    const std::string& project_root,
    std::string& error_msg
) {
    std::error_code ec;
    fs::path cand = fs::weakly_canonical(candidate_path, ec);
    fs::path proj = fs::weakly_canonical(project_root, ec);

    if (cand == proj) {
        error_msg = "RomFS directory cannot point directly to project root";
        return false;
    }
    if (cand == cand.root_path()) {
        error_msg = "RomFS directory cannot point to filesystem root";
        return false;
    }
    if (cand == (proj / ".nxdev")) {
        error_msg = "RomFS directory cannot point to .nxdev internal metadata folder";
        return false;
    }

    return true;
}

std::string RomFsStager::locate_borealis_resources(
    const std::string& project_root,
    const std::string& sdk_root
) {
    std::vector<fs::path> candidates;

    // 1. Explicit SDK root parameter or environment variable
    if (!sdk_root.empty()) {
        candidates.push_back(fs::path(sdk_root) / "share" / "nxdev" / "borealis" / "resources");
        candidates.push_back(fs::path(sdk_root) / "share" / "nxdev" / "resources" / "ui");
    }
    if (const char* env_sdk = std::getenv("NXDEV_SDK_ROOT")) {
        candidates.push_back(fs::path(env_sdk) / "share" / "nxdev" / "borealis" / "resources");
        candidates.push_back(fs::path(env_sdk) / "share" / "nxdev" / "resources" / "ui");
    }

    // 2. Standard system installation paths
    candidates.push_back(fs::path("/opt/nxdev/share/nxdev/borealis/resources"));
    candidates.push_back(fs::path("/opt/nxdev/share/nxdev/resources/ui"));
    candidates.push_back(fs::path("C:/NXDevSDK/share/nxdev/borealis/resources"));
    candidates.push_back(fs::path("C:/NXDevSDK/share/nxdev/resources/ui"));

    // 3. Source-tree layout fallbacks
    if (!project_root.empty()) {
        candidates.push_back(fs::path(project_root) / "third_party" / "borealis" / "resources");
        candidates.push_back(fs::path(project_root) / "sdk" / "modules" / "borealis" / "resources");
    }
#ifdef NXDEV_SOURCE_DIR
    candidates.push_back(fs::path(NXDEV_SOURCE_DIR) / "third_party" / "borealis" / "resources");
    candidates.push_back(fs::path(NXDEV_SOURCE_DIR) / "sdk" / "modules" / "borealis" / "resources");
#endif
    candidates.push_back(fs::current_path() / "third_party" / "borealis" / "resources");

    for (const auto& cand : candidates) {
        if (fs::exists(cand) && fs::is_directory(cand)) {
            return cand.string();
        }
    }

    return "";
}

bool RomFsStager::validate_essential_borealis_resources(
    const std::string& staged_dir,
    std::string& error_msg
) {
    fs::path base = fs::path(staged_dir) / "resources";
    if (!fs::exists(base) || !fs::is_directory(base)) {
        error_msg = "Borealis 'resources' root directory missing in staged RomFS: " + base.string();
        return false;
    }

    // Essential directories and font
    std::vector<fs::path> required = {
        base / "material",
        base / "font",
        base / "i18n"
    };

    for (const auto& req : required) {
        if (!fs::exists(req)) {
            error_msg = "Essential Borealis directory missing in staged RomFS: " + req.string();
            return false;
        }
    }

    return true;
}

std::vector<RomFsLayer> RomFsStager::resolve_manifest_layers(
    const manifest::Manifest& manifest,
    const std::string& project_root,
    const std::string& sdk_root
) {
    std::vector<RomFsLayer> layers;

    // Check if project depends on Borealis UI
    bool has_borealis = false;
    for (const auto& dep : manifest.dependencies()) {
        if (dep.name == "nxdev.borealis" || dep.name == "NXDev::Borealis") {
            has_borealis = true;
            break;
        }
    }

    if (has_borealis) {
        std::string borealis_res_dir = locate_borealis_resources(project_root, sdk_root);
        if (!borealis_res_dir.empty()) {
            layers.push_back(RomFsLayer{
                .name = "borealis",
                .source_path = borealis_res_dir,
                .target_prefix = "resources",
                .priority = 100
            });
        }
    }

    return layers;
}

RomFsStageResult RomFsStager::stage(const RomFsStageRequest& request) {
    RomFsStageResult result;
    result.staged_dir = request.output_dir;

    std::string project_root = request.project_root.empty() ? fs::current_path().string() : request.project_root;

    // Determine output directory
    fs::path out_path;
    if (!request.output_dir.empty()) {
        out_path = fs::absolute(request.output_dir);
    } else {
        out_path = fs::path(project_root) / ".nxdev" / "build" / "debug" / "romfs";
    }
    result.staged_dir = out_path.string();

    // Verify safety of output path
    fs::path proj_nxdev = fs::path(project_root) / ".nxdev";
    std::error_code ec;

    // Collect all layers
    std::vector<RomFsLayer> all_layers = request.framework_layers;

    // Add user RomFS layer if provided and exists
    std::string user_romfs_src;
    if (request.user_romfs_path.has_value() && !request.user_romfs_path->empty()) {
        user_romfs_src = *request.user_romfs_path;
        if (!fs::path(user_romfs_src).is_absolute()) {
            user_romfs_src = (fs::path(project_root) / user_romfs_src).string();
        }

        std::string path_err;
        if (!is_safe_staging_path(user_romfs_src, project_root, path_err)) {
            result.success = false;
            result.error_message = path_err;
            return result;
        }

        if (fs::exists(user_romfs_src) && fs::is_directory(user_romfs_src)) {
            // Check that user RomFS does not overlap or equal staging directory
            fs::path u_canon = fs::weakly_canonical(user_romfs_src, ec);
            fs::path o_canon = fs::weakly_canonical(out_path, ec);
            if (u_canon == o_canon || (o_canon.string().rfind(u_canon.string(), 0) == 0 && o_canon != u_canon)) {
                result.success = false;
                result.error_message = "User RomFS directory overlaps staging destination: " + user_romfs_src;
                return result;
            }

            all_layers.push_back(RomFsLayer{
                .name = "project",
                .source_path = user_romfs_src,
                .target_prefix = "",
                .priority = 1000
            });
        }
    }

    // If no layers exist, no RomFS is needed
    if (all_layers.empty()) {
        result.success = true;
        result.has_romfs = false;
        result.files_copied = 0;
        result.overridden_files = 0;
        return result;
    }

    // Sort layers by priority ascending (framework base -> user overlay)
    std::sort(all_layers.begin(), all_layers.end(), [](const RomFsLayer& a, const RomFsLayer& b) {
        return a.priority < b.priority;
    });

    for (const auto& l : all_layers) {
        result.source_layers.push_back(l.name);
    }

    // Clean staging output directory safely
    if (request.clean) {
        safe_clean_staging_dir(out_path, proj_nxdev);
    }
    fs::create_directories(out_path, ec);

    // Track staged files: destination relative path -> manifest entry
    std::map<std::string, RomFsManifestEntry> staged_files;
    std::set<std::string> lower_case_paths; // For case collision detection

    for (const auto& layer : all_layers) {
        fs::path src_root(layer.source_path);
        if (!fs::exists(src_root) || !fs::is_directory(src_root)) {
            continue;
        }

        // Collect and sort files deterministically
        std::vector<fs::path> layer_files;
        for (const auto& entry : fs::recursive_directory_iterator(src_root, fs::directory_options::skip_permission_denied)) {
            // Symlink escape check
            if (entry.is_symlink() || fs::is_symlink(entry.symlink_status())) {
                fs::path target = fs::canonical(entry.path(), ec);
                fs::path canon_root = fs::canonical(src_root, ec);
                if (ec || target.string().rfind(canon_root.string(), 0) != 0) {
                    result.success = false;
                    result.error_message = "RomFS resource symlink escapes source root: " + entry.path().string();
                    return result;
                }
            }

            if (entry.is_regular_file()) {
                layer_files.push_back(entry.path());
            }
        }
        std::sort(layer_files.begin(), layer_files.end());

        for (const auto& file_path : layer_files) {
            // Symlink escape check for resolved files
            if (fs::is_symlink(file_path)) {
                fs::path target = fs::canonical(file_path, ec);
                fs::path canon_root = fs::canonical(src_root, ec);
                if (ec || target.string().rfind(canon_root.string(), 0) != 0) {
                    result.success = false;
                    result.error_message = "RomFS resource symlink escapes source root: " + file_path.string();
                    return result;
                }
            }

            fs::path rel_from_src = fs::relative(file_path, src_root);
            
            // Path traversal prevention
            std::string rel_str = rel_from_src.string();
            if (rel_str.find("..") != std::string::npos || rel_str.starts_with("/")) {
                result.success = false;
                result.error_message = "Path traversal detected in RomFS source file: " + rel_str;
                return result;
            }

            fs::path dest_rel = layer.target_prefix.empty() ? rel_from_src : (fs::path(layer.target_prefix) / rel_from_src);
            std::string dest_rel_str = dest_rel.lexically_normal().string();

            // Case collision check
            std::string lower_dest = to_lower_str(dest_rel_str);
            if (lower_case_paths.find(lower_dest) != lower_case_paths.end() && staged_files.find(dest_rel_str) == staged_files.end()) {
                result.warnings.push_back("Potential case-only filename collision in RomFS: " + dest_rel_str);
            }
            lower_case_paths.insert(lower_dest);

            fs::path final_dest_path = out_path / dest_rel;
            fs::create_directories(final_dest_path.parent_path(), ec);

            std::string overridden_layer;
            auto existing_it = staged_files.find(dest_rel_str);
            if (existing_it != staged_files.end()) {
                overridden_layer = existing_it->second.source_layer;
                result.overridden_files++;

                if (request.strict_collision) {
                    result.success = false;
                    result.error_message = "Strict collision error: File '" + dest_rel_str + "' from layer '" +
                                          layer.name + "' collides with '" + overridden_layer + "'";
                    return result;
                }

                if (request.verbose) {
                    std::cout << "[RomFS] Overlay: " << dest_rel_str << " (" << overridden_layer << " -> " << layer.name << ")\n";
                }
            }

            // Copy file to stage
            fs::copy_file(file_path, final_dest_path, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                result.success = false;
                result.error_message = "Failed to copy file to RomFS staging: " + file_path.string() + " -> " + final_dest_path.string();
                return result;
            }

            std::string hash = compute_file_sha256(final_dest_path.string());
            size_t sz = fs::file_size(final_dest_path, ec);

            staged_files[dest_rel_str] = RomFsManifestEntry{
                .relative_path = dest_rel_str,
                .source_layer = layer.name,
                .source_file = file_path.string(),
                .sha256 = hash,
                .size_bytes = sz,
                .overridden_layer = overridden_layer
            };
        }
    }

    result.files_copied = staged_files.size();
    result.has_romfs = (result.files_copied > 0);

    for (const auto& [_, entry] : staged_files) {
        result.entries.push_back(entry);
    }

    // Compute combined deterministic fingerprint
    std::ostringstream fp_stream;
    for (const auto& entry : result.entries) {
        fp_stream << entry.relative_path << ":" << entry.sha256 << ":" << entry.source_layer << "\n";
    }
    std::string fp_data = fp_stream.str();
    result.fingerprint = sha256_internal::hash_bytes(reinterpret_cast<const uint8_t*>(fp_data.data()), fp_data.size());

    // Generate romfs-manifest.json in the parent build directory
    fs::path manifest_file = out_path.parent_path() / "romfs-manifest.json";
    result.manifest_path = manifest_file.string();

    {
        std::ofstream mf(manifest_file);
        if (mf.is_open()) {
            mf << result.to_json();
        }
    }

    // Validate essential Borealis files if borealis layer was included
    bool has_borealis = std::find(result.source_layers.begin(), result.source_layers.end(), "borealis") != result.source_layers.end();
    if (has_borealis) {
        std::string borealis_err;
        if (!validate_essential_borealis_resources(out_path.string(), borealis_err)) {
            result.warnings.push_back(borealis_err);
        }
    }

    result.success = true;
    return result;
}

std::string RomFsStageResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"status\": \"" << (success ? "success" : "error") << "\",\n";
    ss << "  \"hasRomFS\": " << (has_romfs ? "true" : "false") << ",\n";
    ss << "  \"stagedDirectory\": \"" << escape_json_str(staged_dir) << "\",\n";
    ss << "  \"manifestPath\": \"" << escape_json_str(manifest_path) << "\",\n";
    ss << "  \"fingerprint\": \"" << escape_json_str(fingerprint) << "\",\n";
    ss << "  \"totalFiles\": " << files_copied << ",\n";
    ss << "  \"overriddenFiles\": " << overridden_files << ",\n";
    ss << "  \"layers\": [";
    for (size_t i = 0; i < source_layers.size(); ++i) {
        ss << "\"" << escape_json_str(source_layers[i]) << "\"" << (i + 1 < source_layers.size() ? ", " : "");
    }
    ss << "],\n";
    ss << "  \"files\": [\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        ss << "    {\n";
        ss << "      \"path\": \"" << escape_json_str(e.relative_path) << "\",\n";
        ss << "      \"sourceLayer\": \"" << escape_json_str(e.source_layer) << "\",\n";
        ss << "      \"sourceFile\": \"" << escape_json_str(e.source_file) << "\",\n";
        ss << "      \"sha256\": \"" << escape_json_str(e.sha256) << "\",\n";
        ss << "      \"sizeBytes\": " << e.size_bytes;
        if (!e.overridden_layer.empty()) {
            ss << ",\n      \"overriddenLayer\": \"" << escape_json_str(e.overridden_layer) << "\"\n";
        } else {
            ss << "\n";
        }
        ss << "    }" << (i + 1 < entries.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

} // namespace nxdev::pack
