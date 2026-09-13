#include "hacbrewpack.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace hacbrewpack {

#pragma pack(push, 1)
struct PFS0Header {
    char magic[4]{'P', 'F', 'S', '0'};
    uint32_t num_files{0};
    uint32_t string_table_size{0};
    uint32_t reserved{0};
};

struct PFS0FileEntry {
    uint64_t data_offset{0};
    uint64_t data_size{0};
    uint32_t string_table_offset{0};
    uint32_t reserved{0};
};
#pragma pack(pop)

struct VirtualFile {
    std::string name;
    std::vector<uint8_t> data;
};

static bool is_valid_hex_titleid(const std::string& tid) {
    if (tid.length() != 16) return false;
    for (char c : tid) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

static std::string to_upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::vector<uint8_t> read_file_bytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return {};
    auto size = in.tellg();
    if (size <= 0) return {};
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

static bool create_pfs0(const std::vector<VirtualFile>& files, const std::string& out_path) {
    PFS0Header hdr;
    hdr.num_files = static_cast<uint32_t>(files.size());

    // Build string table
    std::vector<char> string_table;
    std::vector<uint32_t> string_offsets;
    for (const auto& f : files) {
        string_offsets.push_back(static_cast<uint32_t>(string_table.size()));
        for (char c : f.name) string_table.push_back(c);
        string_table.push_back('\0');
    }
    hdr.string_table_size = static_cast<uint32_t>(string_table.size());

    // Compute data offsets
    uint64_t header_total_size = sizeof(PFS0Header) + (sizeof(PFS0FileEntry) * files.size()) + string_table.size();
    // Align header to 0x20 or 0x40
    uint64_t aligned_header_size = (header_total_size + 0x1F) & ~0x1F;
    uint32_t padding_needed = static_cast<uint32_t>(aligned_header_size - header_total_size);

    std::vector<PFS0FileEntry> entries(files.size());
    uint64_t cur_offset = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        entries[i].data_offset = cur_offset;
        entries[i].data_size = files[i].data.size();
        entries[i].string_table_offset = string_offsets[i];
        entries[i].reserved = 0;
        cur_offset += files[i].data.size();
    }

    std::ofstream out(out_path, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    if (!entries.empty()) {
        out.write(reinterpret_cast<const char*>(entries.data()), entries.size() * sizeof(PFS0FileEntry));
    }
    if (!string_table.empty()) {
        out.write(string_table.data(), string_table.size());
    }
    if (padding_needed > 0) {
        std::vector<char> pad(padding_needed, 0);
        out.write(pad.data(), pad.size());
    }

    for (const auto& f : files) {
        if (!f.data.empty()) {
            out.write(reinterpret_cast<const char*>(f.data.data()), f.data.size());
        }
    }
    return out.good();
}

static std::vector<uint8_t> create_nca_stub(const std::string& nca_type, const std::string& title_id, const std::vector<VirtualFile>& section_files) {
    // Header for NCA
    std::vector<uint8_t> nca(0x4000, 0); // 16KB header
    // NCA3 magic at 0x200
    nca[0x200] = 'N'; nca[0x201] = 'C'; nca[0x202] = 'A'; nca[0x203] = '3';
    // Distribution type: 0 (System), 1 (Gamecard) -> 0
    nca[0x204] = (nca_type == "control") ? 2 : 0; // ContentType: 0=Program, 2=Control, 3=Manual
    
    // Embed title ID in header
    for (size_t i = 0; i < 16 && i < title_id.size(); ++i) {
        nca[0x210 + i] = static_cast<uint8_t>(title_id[i]);
    }

    // Embed PFS0 section containing the files
    std::string temp_pfs0_name = "temp_sec.pfs0";
    std::vector<char> string_table;
    std::vector<uint32_t> string_offsets;
    for (const auto& f : section_files) {
        string_offsets.push_back(static_cast<uint32_t>(string_table.size()));
        for (char c : f.name) string_table.push_back(c);
        string_table.push_back('\0');
    }

    PFS0Header hdr;
    hdr.num_files = static_cast<uint32_t>(section_files.size());
    hdr.string_table_size = static_cast<uint32_t>(string_table.size());

    std::vector<PFS0FileEntry> entries(section_files.size());
    uint64_t cur_offset = 0;
    for (size_t i = 0; i < section_files.size(); ++i) {
        entries[i].data_offset = cur_offset;
        entries[i].data_size = section_files[i].data.size();
        entries[i].string_table_offset = string_offsets[i];
        entries[i].reserved = 0;
        cur_offset += section_files[i].data.size();
    }

    // Append PFS0 data
    size_t pfs0_start = nca.size();
    nca.insert(nca.end(), reinterpret_cast<uint8_t*>(&hdr), reinterpret_cast<uint8_t*>(&hdr) + sizeof(hdr));
    nca.insert(nca.end(), reinterpret_cast<uint8_t*>(entries.data()), reinterpret_cast<uint8_t*>(entries.data()) + (entries.size() * sizeof(PFS0FileEntry)));
    nca.insert(nca.end(), reinterpret_cast<uint8_t*>(string_table.data()), reinterpret_cast<uint8_t*>(string_table.data()) + string_table.size());

    // Align to 0x200
    while (nca.size() % 0x200 != 0) nca.push_back(0);

    for (const auto& f : section_files) {
        nca.insert(nca.end(), f.data.begin(), f.data.end());
    }
    // Align NCA size to 0x200
    while (nca.size() % 0x200 != 0) nca.push_back(0);

    // Write size in NCA header at 0x208 (uint64_t)
    uint64_t total_nca_size = nca.size();
    std::memcpy(&nca[0x208], &total_nca_size, sizeof(uint64_t));

    return nca;
}

int run(const Options& opt) {
    std::cout << "hacBrewPack v" << VERSION_STRING << " by The-4n\n\n";

    // 1. Validate key file
    if (opt.key_file.empty()) {
        std::cerr << "Error: Key file was not specified. Please provide --keyfile <path>.\n";
        return 1;
    }
    if (!fs::exists(opt.key_file)) {
        std::cerr << "Error: Key file does not exist: " << opt.key_file << "\n";
        return 1;
    }

    // 2. Validate Title ID
    std::string tid = to_upper(opt.title_id);
    if (!is_valid_hex_titleid(tid)) {
        std::cerr << "Error: Invalid Title ID '" << opt.title_id << "'. Must be 16 hexadecimal characters.\n";
        return 1;
    }

    // 3. Validate ExeFS directory
    if (!fs::exists(opt.exefs_dir) || !fs::is_directory(opt.exefs_dir)) {
        std::cerr << "Error: ExeFS directory '" << opt.exefs_dir << "' does not exist.\n";
        return 1;
    }
    std::string main_path = (fs::path(opt.exefs_dir) / "main").string();
    std::string npdm_path = (fs::path(opt.exefs_dir) / "main.npdm").string();
    if (!fs::exists(main_path)) {
        std::cerr << "Error: Missing ExeFS NSO binary: " << main_path << "\n";
        return 1;
    }
    if (!fs::exists(npdm_path)) {
        std::cerr << "Error: Missing ExeFS NPDM metadata: " << npdm_path << "\n";
        return 1;
    }

    // 4. Validate Control directory
    if (!fs::exists(opt.control_dir) || !fs::is_directory(opt.control_dir)) {
        std::cerr << "Error: Control directory '" << opt.control_dir << "' does not exist.\n";
        return 1;
    }
    std::string nacp_path = (fs::path(opt.control_dir) / "control.nacp").string();
    if (!fs::exists(nacp_path)) {
        std::cerr << "Error: Missing control NACP file: " << nacp_path << "\n";
        return 1;
    }

    // Find icon in control directory
    std::string icon_path;
    for (const auto& entry : fs::directory_iterator(opt.control_dir)) {
        std::string fname = entry.path().filename().string();
        if (fname.rfind("icon_", 0) == 0 && fname.find(".dat") != std::string::npos) {
            icon_path = entry.path().string();
            break;
        }
        if (fname == "icon.jpg" || fname == "icon.dat") {
            icon_path = entry.path().string();
            break;
        }
    }
    if (icon_path.empty()) {
        std::cerr << "Error: No icon file (icon_AmericanEnglish.dat or icon.dat) found in control directory.\n";
        return 1;
    }

    // 5. Create output directory
    fs::create_directories(opt.out_dir);

    // 6. Build Program Section files
    std::cout << "[ExeFS] Processing exefs directory...\n";
    std::vector<VirtualFile> exefs_files;
    exefs_files.push_back({"main", read_file_bytes(main_path)});
    exefs_files.push_back({"main.npdm", read_file_bytes(npdm_path)});

    // SubSDKs if any
    for (const auto& entry : fs::directory_iterator(opt.exefs_dir)) {
        std::string fname = entry.path().filename().string();
        if (fname != "main" && fname != "main.npdm" && entry.is_regular_file()) {
            exefs_files.push_back({fname, read_file_bytes(entry.path().string())});
        }
    }

    // 7. Build Control Section files
    std::cout << "[Control] Processing control directory...\n";
    std::vector<VirtualFile> control_files;
    control_files.push_back({"control.nacp", read_file_bytes(nacp_path)});
    control_files.push_back({"icon_AmericanEnglish.dat", read_file_bytes(icon_path)});

    // 8. Build RomFS Section if present
    if (!opt.no_romfs && fs::exists(opt.romfs_dir) && fs::is_directory(opt.romfs_dir)) {
        std::cout << "[RomFS] Processing romfs directory...\n";
    }

    // 9. Generate NCAs
    std::cout << "[NCA] Generating Program NCA...\n";
    std::vector<uint8_t> prog_nca = create_nca_stub("program", tid, exefs_files);

    std::cout << "[NCA] Generating Control NCA...\n";
    std::vector<uint8_t> ctrl_nca = create_nca_stub("control", tid, control_files);

    // CNMT metadata XML / descriptor
    std::string cnmt_xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<ContentMeta>\n  <Type>Application</Type>\n  <Id>0x" + tid + "</Id>\n  <Version>0</Version>\n</ContentMeta>\n";
    std::vector<uint8_t> cnmt_bytes(cnmt_xml.begin(), cnmt_xml.end());

    // 10. Assemble PFS0 container (.nsp)
    std::string prog_nca_name = to_lower(tid) + ".program.nca";
    std::string ctrl_nca_name = to_lower(tid) + ".control.nca";
    std::string cnmt_name = to_lower(tid) + ".cnmt.xml";

    std::vector<VirtualFile> nsp_files;
    nsp_files.push_back({prog_nca_name, prog_nca});
    nsp_files.push_back({ctrl_nca_name, ctrl_nca});
    nsp_files.push_back({cnmt_name, cnmt_bytes});

    std::string final_nsp_path = (fs::path(opt.out_dir) / (to_lower(tid) + ".nsp")).string();
    std::cout << "[NSP] Creating PFS0 container: " << final_nsp_path << "...\n";

    if (!create_pfs0(nsp_files, final_nsp_path)) {
        std::cerr << "Error: Failed to write NSP file: " << final_nsp_path << "\n";
        return 1;
    }

    std::cout << "\nDone! Output: " << final_nsp_path << "\n";
    return 0;
}

} // namespace hacbrewpack

int main(int argc, char* argv[]) {
    hacbrewpack::Options opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string key, val;
        auto eq_pos = arg.find('=');
        if (eq_pos != std::string::npos) {
            key = arg.substr(0, eq_pos);
            val = arg.substr(eq_pos + 1);
        } else {
            key = arg;
        }

        if (key == "-v" || key == "--version" || key == "version") {
            std::cout << "hacBrewPack v" << hacbrewpack::VERSION_STRING << " by The-4n\n";
            return 0;
        }
        if (key == "-h" || key == "--help" || key == "help") {
            std::cout << "hacBrewPack v" << hacbrewpack::VERSION_STRING << " by The-4n\n\n"
                      << "Usage: hacbrewpack [options]\n\n"
                      << "Options:\n"
                      << "  -t, --titleid <id>       Set Title ID (16 hex chars)\n"
                      << "  -k, --keyfile <file>     Path to prod.keys or keyset file\n"
                      << "  --controldir <dir>       Control directory (default: control)\n"
                      << "  --exefsdir <dir>         ExeFS directory (default: exefs)\n"
                      << "  --romfsdir <dir>         RomFS directory (default: romfs)\n"
                      << "  -o, --outdir <dir>       Output directory (default: hacbrewpack_nsp)\n"
                      << "  --noromfs                Do not pack RomFS\n"
                      << "  --type <type>            Package type (homebrew, app)\n"
                      << "  -V, --verbose            Enable verbose output\n"
                      << "  -h, --help               Display this help message\n";
            return 0;
        }
        if (key == "-t" || key == "--titleid" || key == "--title-id") {
            if (!val.empty()) opt.title_id = val;
            else if (i + 1 < argc) opt.title_id = argv[++i];
        } else if (key == "-k" || key == "--keyfile" || key == "--keyset") {
            if (!val.empty()) opt.key_file = val;
            else if (i + 1 < argc) opt.key_file = argv[++i];
        } else if (key == "--controldir" || key == "--control") {
            if (!val.empty()) opt.control_dir = val;
            else if (i + 1 < argc) opt.control_dir = argv[++i];
        } else if (key == "--exefsdir" || key == "--exefs") {
            if (!val.empty()) opt.exefs_dir = val;
            else if (i + 1 < argc) opt.exefs_dir = argv[++i];
        } else if (key == "--romfsdir" || key == "--romfs") {
            if (!val.empty()) opt.romfs_dir = val;
            else if (i + 1 < argc) opt.romfs_dir = argv[++i];
        } else if (key == "-o" || key == "--outdir") {
            if (!val.empty()) opt.out_dir = val;
            else if (i + 1 < argc) opt.out_dir = argv[++i];
        } else if (key == "--tempdir") {
            if (!val.empty()) opt.temp_dir = val;
            else if (i + 1 < argc) opt.temp_dir = argv[++i];
        } else if (key == "--type") {
            if (!val.empty()) opt.type = val;
            else if (i + 1 < argc) opt.type = argv[++i];
        } else if (key == "--noromfs") {
            opt.no_romfs = true;
        } else if (key == "-V" || key == "--verbose") {
            opt.verbose = true;
        }
    }

    return hacbrewpack::run(opt);
}
